// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/bnet.hpp>
#include <bnet/core/Place.hpp>
#include <bnet/execution/ActionExecutor.hpp>
#include <bnet/places/ActionPlace.hpp>
#include <bnet/places/EntrypointPlace.hpp>
#include <bnet/places/ExitLoggerPlace.hpp>
#include <bnet/places/PlainPlace.hpp>
#include <bnet/places/ResourcePoolPlace.hpp>
#include <bnet/places/WaitWithTimeoutPlace.hpp>

#include <cassert>
#include <iostream>
#include <thread>

void testPlainPlace()
{
    std::cout << "Testing PlainPlace..." << std::endl;

    bnet::core::Place place("p1");
    bnet::places::PlainPlace plainPlace(place);

    assert(plainPlace.typeName() == "PlainPlace");
    assert(&plainPlace.place() == &place);

    // PlainPlace has no special behavior
    bnet::Token token;
    plainPlace.onTokenEnter(std::move(token));
    plainPlace.tick(1);

    std::cout << "  PASSED" << std::endl;
}

void testEntrypointPlace()
{
    std::cout << "Testing EntrypointPlace..." << std::endl;

    bnet::core::Place place("entry");
    bnet::places::EntrypointPlace entryPlace(place);

    assert(entryPlace.typeName() == "EntrypointPlace");
    assert(entryPlace.injectedCount() == 0);

    // Inject without validator
    bnet::Token t1;
    auto id1 = entryPlace.inject(std::move(t1));
    assert(id1 != 0);
    assert(entryPlace.injectedCount() == 1);
    assert(place.tokenCount() == 1);

    // Add validator that rejects
    entryPlace.setValidator([](const bnet::Token&) { return false; });

    bnet::Token t2;
    auto id2 = entryPlace.inject(std::move(t2));
    assert(id2 == 0);
    assert(entryPlace.injectedCount() == 1);

    // Validator that accepts
    entryPlace.setValidator([](const bnet::Token&) { return true; });

    bnet::Token t3;
    auto id3 = entryPlace.inject(std::move(t3));
    assert(id3 != 0);
    assert(entryPlace.injectedCount() == 2);

    std::cout << "  PASSED" << std::endl;
}

void testResourcePoolPlace()
{
    std::cout << "Testing ResourcePoolPlace..." << std::endl;

    bnet::core::Place place("pool");
    bnet::places::ResourcePoolPlace poolPlace(place, 3);

    assert(poolPlace.typeName() == "ResourcePoolPlace");
    assert(poolPlace.poolSize() == 3);
    assert(poolPlace.availableCount() == 3);

    // Acquire resource
    auto r1 = poolPlace.acquire();
    assert(r1.has_value());
    assert(poolPlace.availableCount() == 2);

    auto r2 = poolPlace.acquire();
    auto r3 = poolPlace.acquire();
    assert(poolPlace.availableCount() == 0);

    // No more available
    auto r4 = poolPlace.acquire();
    assert(!r4.has_value());

    // Release one back
    poolPlace.release(std::move(r1->second));
    assert(poolPlace.availableCount() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testExitLoggerPlace()
{
    std::cout << "Testing ExitLoggerPlace..." << std::endl;

    bnet::core::Place place("exit");
    bnet::places::ExitLoggerPlace exitPlace(place);

    assert(exitPlace.typeName() == "ExitLoggerPlace");
    assert(exitPlace.exitCount() == 0);

    int logCount = 0;
    exitPlace.setLogger([&](const std::string& placeId, const bnet::Token&)
    {
        assert(placeId == "exit");
        ++logCount;
    });

    // Token enters via onTokenEnter (direct call)
    bnet::Token t1;
    exitPlace.onTokenEnter(std::move(t1));
    assert(exitPlace.exitCount() == 1);
    assert(logCount == 1);

    // Token enters via place (processed in tick)
    place.addToken(bnet::Token{});
    exitPlace.tick(1);
    assert(exitPlace.exitCount() == 2);
    assert(logCount == 2);

    std::cout << "  PASSED" << std::endl;
}

void testWaitWithTimeoutPlace()
{
    std::cout << "Testing WaitWithTimeoutPlace..." << std::endl;

    bnet::core::Place place("wait");
    bnet::places::WaitWithTimeoutPlace waitPlace(place, std::chrono::milliseconds(50));

    assert(waitPlace.typeName() == "WaitWithTimeoutPlace");
    assert(place.hasSubplaces());

    // Token enters, waits
    bnet::Token t1;
    waitPlace.onTokenEnter(std::move(t1));
    assert(place.subplace(bnet::core::Subplace::Main).size() == 1);

    // Tick without timeout
    waitPlace.tick(1);
    assert(place.subplace(bnet::core::Subplace::Main).size() == 1);
    assert(place.subplace(bnet::core::Subplace::Failure).size() == 0);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    waitPlace.tick(2);
    assert(place.subplace(bnet::core::Subplace::Main).size() == 0);
    assert(place.subplace(bnet::core::Subplace::Failure).size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testWaitWithCondition()
{
    std::cout << "Testing WaitWithTimeoutPlace with condition..." << std::endl;

    bnet::core::Place place("wait");
    bnet::places::WaitWithTimeoutPlace waitPlace(place, std::chrono::milliseconds(1000));

    bool conditionMet = false;
    waitPlace.setCondition([&](const bnet::Token&) { return conditionMet; });

    bnet::Token t1;
    waitPlace.onTokenEnter(std::move(t1));

    // Tick without condition met
    waitPlace.tick(1);
    assert(place.subplace(bnet::core::Subplace::Main).size() == 1);
    assert(place.subplace(bnet::core::Subplace::Success).size() == 0);

    // Set condition and tick
    conditionMet = true;
    waitPlace.tick(2);
    assert(place.subplace(bnet::core::Subplace::Main).size() == 0);
    assert(place.subplace(bnet::core::Subplace::Success).size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testActionPlace()
{
    std::cout << "Testing ActionPlace..." << std::endl;

    bnet::core::Place place("action");
    bnet::execution::ActionExecutor executor;

    bnet::places::ActionConfig config;
    config.actionName = "test_action";
    config.retryPolicy = bnet::execution::RetryPolicy::noRetry();

    bnet::places::ActionPlace actionPlace(place, config, executor);

    assert(actionPlace.typeName() == "ActionPlace");
    assert(place.hasSubplaces());

    // Set invoker that succeeds
    actionPlace.setInvoker([](bnet::ActorBase*, bnet::Token&) -> bnet::ActionResult
    {
        return bnet::ActionResult::success();
    });

    // Token enters, action starts
    bnet::Token t1;
    actionPlace.onTokenEnter(std::move(t1));

    // Poll executor to process
    executor.poll();

    assert(place.subplace(bnet::core::Subplace::Success).size() == 1);
    assert(place.subplace(bnet::core::Subplace::Failure).size() == 0);
    assert(place.subplace(bnet::core::Subplace::Error).size() == 0);

    std::cout << "  PASSED" << std::endl;
}

void testActionPlaceFailure()
{
    std::cout << "Testing ActionPlace failure..." << std::endl;

    bnet::core::Place place("action");
    bnet::execution::ActionExecutor executor;

    bnet::places::ActionConfig config;
    config.actionName = "failing_action";
    config.retryPolicy = bnet::execution::RetryPolicy::noRetry();

    bnet::places::ActionPlace actionPlace(place, config, executor);

    // Set invoker that fails
    actionPlace.setInvoker([](bnet::ActorBase*, bnet::Token&) -> bnet::ActionResult
    {
        return bnet::ActionResult::failure("Test failure");
    });

    bnet::Token t1;
    actionPlace.onTokenEnter(std::move(t1));
    executor.poll();

    assert(place.subplace(bnet::core::Subplace::Success).size() == 0);
    assert(place.subplace(bnet::core::Subplace::Failure).size() == 1);

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Place Types Tests ===" << std::endl;

    testPlainPlace();
    testEntrypointPlace();
    testResourcePoolPlace();
    testExitLoggerPlace();
    testWaitWithTimeoutPlace();
    testWaitWithCondition();
    testActionPlace();
    testActionPlaceFailure();

    std::cout << "\n=== All place types tests passed ===" << std::endl;
    return 0;
}
