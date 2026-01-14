// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/bnet.hpp>
#include <bnet/execution/ActionContext.hpp>
#include <bnet/execution/ActionExecutor.hpp>
#include <bnet/execution/RetryPolicy.hpp>

#include <cassert>
#include <iostream>
#include <thread>

class TestActor : public bnet::ActorBase
{
public:
    int callCount{0};
    bnet::ActionResult::Status nextStatus{bnet::ActionResult::Status::Success};

    bnet::ActionResult doAction(bnet::Token& /*token*/)
    {
        ++callCount;
        switch (nextStatus)
        {
            case bnet::ActionResult::Status::Success:
                return bnet::ActionResult::success();
            case bnet::ActionResult::Status::Failure:
                return bnet::ActionResult::failure("Test failure");
            case bnet::ActionResult::Status::Error:
                return bnet::ActionResult::errorMessage("Test error");
            case bnet::ActionResult::Status::InProgress:
                return bnet::ActionResult::inProgress();
        }
        return bnet::ActionResult::success();
    }
};

void testRetryPolicy()
{
    std::cout << "Testing RetryPolicy..." << std::endl;

    bnet::execution::RetryPolicy defaultPolicy;
    assert(defaultPolicy.maxRetries == 3);
    assert(defaultPolicy.timeout == std::chrono::milliseconds(30000));
    assert(defaultPolicy.retryOnError == true);
    assert(defaultPolicy.retryOnFailure == false);

    auto noRetry = bnet::execution::RetryPolicy::noRetry();
    assert(noRetry.maxRetries == 0);
    assert(noRetry.retryOnError == false);

    auto immediate = bnet::execution::RetryPolicy::immediate(5);
    assert(immediate.maxRetries == 5);
    assert(immediate.retryDelay == std::chrono::milliseconds(0));

    std::cout << "  PASSED" << std::endl;
}

void testActionContext()
{
    std::cout << "Testing ActionContext..." << std::endl;

    bool callbackInvoked = false;
    bnet::ActionResult receivedResult;

    auto callback = [&](bnet::execution::ActionId id, bnet::ActionResult result, bnet::Token /*token*/)
    {
        callbackInvoked = true;
        receivedResult = std::move(result);
        assert(id == 1);
    };

    bnet::Token token;
    bnet::execution::RetryPolicy policy;
    bnet::execution::ActionContext ctx(1, "test_action", std::move(token), policy, callback);

    assert(ctx.id() == 1);
    assert(ctx.actionName() == "test_action");
    assert(ctx.state() == bnet::execution::ActionState::Pending);
    assert(ctx.attemptCount() == 0);

    // Start action
    ctx.start();
    assert(ctx.state() == bnet::execution::ActionState::Running);
    assert(ctx.attemptCount() == 1);

    // Complete with success
    ctx.update(bnet::ActionResult::success());
    assert(ctx.state() == bnet::execution::ActionState::Completed);

    // Invoke callback
    ctx.invokeCallback();
    assert(callbackInvoked);
    assert(receivedResult.status() == bnet::ActionResult::Status::Success);

    std::cout << "  PASSED" << std::endl;
}

void testActionContextRetry()
{
    std::cout << "Testing ActionContext retry..." << std::endl;

    bnet::execution::RetryPolicy policy;
    policy.maxRetries = 2;
    policy.retryDelay = std::chrono::milliseconds(0);

    bnet::Token token;
    bnet::execution::ActionContext ctx(1, "test_action", std::move(token), policy, nullptr);

    // First attempt - error
    ctx.start();
    ctx.update(bnet::ActionResult::errorMessage("First error"));
    assert(ctx.state() == bnet::execution::ActionState::Error);
    assert(ctx.canRetry());

    ctx.scheduleRetry();
    assert(ctx.state() == bnet::execution::ActionState::Pending);

    // Second attempt - error
    ctx.start();
    ctx.update(bnet::ActionResult::errorMessage("Second error"));
    assert(ctx.canRetry());

    ctx.scheduleRetry();
    ctx.start();

    // Third attempt - error (no more retries)
    ctx.update(bnet::ActionResult::errorMessage("Third error"));
    assert(!ctx.canRetry());

    std::cout << "  PASSED" << std::endl;
}

void testActionExecutor()
{
    std::cout << "Testing ActionExecutor..." << std::endl;

    bnet::execution::ActionExecutor executor;
    assert(executor.inFlightCount() == 0);
    assert(!executor.hasInFlightActions());

    TestActor actor;
    bool completed = false;

    auto invoker = [](bnet::ActorBase* a, bnet::Token& t) -> bnet::ActionResult
    {
        return static_cast<TestActor*>(a)->doAction(t);
    };

    auto callback = [&](bnet::execution::ActionId /*id*/, bnet::ActionResult result, bnet::Token /*token*/)
    {
        completed = true;
        assert(result.status() == bnet::ActionResult::Status::Success);
    };

    bnet::Token token;
    auto id = executor.startAction("test", std::move(token), &actor, invoker, bnet::execution::RetryPolicy::noRetry(), callback);
    assert(id == 1);
    assert(executor.inFlightCount() == 1);

    // Poll to execute
    executor.poll();

    assert(completed);
    assert(actor.callCount == 1);
    assert(executor.inFlightCount() == 0);

    std::cout << "  PASSED" << std::endl;
}

void testActionExecutorRetry()
{
    std::cout << "Testing ActionExecutor retry..." << std::endl;

    bnet::execution::ActionExecutor executor;
    TestActor actor;
    actor.nextStatus = bnet::ActionResult::Status::Error;

    int completionCount = 0;

    auto invoker = [](bnet::ActorBase* a, bnet::Token& t) -> bnet::ActionResult
    {
        return static_cast<TestActor*>(a)->doAction(t);
    };

    auto callback = [&](bnet::execution::ActionId /*id*/, bnet::ActionResult /*result*/, bnet::Token /*token*/)
    {
        ++completionCount;
    };

    bnet::execution::RetryPolicy policy;
    policy.maxRetries = 2;
    policy.retryDelay = std::chrono::milliseconds(0);
    policy.retryOnError = true;

    bnet::Token token;
    executor.startAction("test", std::move(token), &actor, invoker, policy, callback);

    // Poll multiple times to trigger retries
    for (int i = 0; i < 5; ++i)
    {
        executor.poll();
    }

    // Should have been called 3 times (1 initial + 2 retries)
    assert(actor.callCount == 3);
    assert(completionCount == 1);
    assert(executor.inFlightCount() == 0);

    std::cout << "  PASSED" << std::endl;
}

void testActionExecutorCancel()
{
    std::cout << "Testing ActionExecutor cancel..." << std::endl;

    bnet::execution::ActionExecutor executor;
    TestActor actor;
    actor.nextStatus = bnet::ActionResult::Status::InProgress;

    bool completed = false;

    auto invoker = [](bnet::ActorBase* a, bnet::Token& t) -> bnet::ActionResult
    {
        return static_cast<TestActor*>(a)->doAction(t);
    };

    auto callback = [&](bnet::execution::ActionId /*id*/, bnet::ActionResult result, bnet::Token /*token*/)
    {
        completed = true;
        assert(result.status() == bnet::ActionResult::Status::InProgress);
    };

    bnet::Token token;
    auto id = executor.startAction("test", std::move(token), &actor, invoker, bnet::execution::RetryPolicy::noRetry(), callback);

    executor.poll();  // Start execution
    assert(executor.hasInFlightActions());

    executor.cancel(id);
    executor.poll();  // Process cancellation

    assert(completed);
    assert(!executor.hasInFlightActions());

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Execution Tests ===" << std::endl;

    testRetryPolicy();
    testActionContext();
    testActionContextRetry();
    testActionExecutor();
    testActionExecutorRetry();
    testActionExecutorCancel();

    std::cout << "\n=== All execution tests passed ===" << std::endl;
    return 0;
}
