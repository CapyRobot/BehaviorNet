// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/execution/ActionExecutor.hpp"

#include <algorithm>

namespace bnet::execution {

ActionId ActionExecutor::startAction(
    std::string actionName,
    Token token,
    ActorBase* actor,
    ActionInvoker invoker,
    RetryPolicy policy,
    ActionCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    ActionId id = nextId_++;
    ActionContext ctx(id, std::move(actionName), std::move(token), policy, std::move(callback));

    inFlight_.emplace(id, InFlightAction{std::move(ctx), actor, std::move(invoker)});

    return id;
}

void ActionExecutor::poll()
{
    std::vector<ActionId> completed;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [id, action] : inFlight_)
        {
            processAction(action.context, action.actor, action.invoker);

            auto state = action.context.state();
            if (state == ActionState::Completed ||
                state == ActionState::Cancelled ||
                state == ActionState::TimedOut ||
                ((state == ActionState::Failed || state == ActionState::Error) && !action.context.canRetry()))
            {
                completed.push_back(id);
            }
        }
    }

    // Invoke callbacks and remove outside lock
    for (ActionId id : completed)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = inFlight_.find(id);
        if (it != inFlight_.end())
        {
            it->second.context.invokeCallback();
            inFlight_.erase(it);
        }
    }
}

void ActionExecutor::processAction(ActionContext& ctx, ActorBase* actor, const ActionInvoker& invoker)
{
    switch (ctx.state())
    {
        case ActionState::Pending:
            if (ctx.isReadyForRetry() || ctx.attemptCount() == 0)
            {
                ctx.start();
                if (invoker)
                {
                    auto result = invoker(actor, ctx.token());
                    ctx.update(std::move(result));

                    // Check if we need to retry after completion
                    if ((ctx.state() == ActionState::Failed || ctx.state() == ActionState::Error) && ctx.canRetry())
                    {
                        ctx.scheduleRetry();
                    }
                }
            }
            break;

        case ActionState::Running:
            if (ctx.isTimedOut())
            {
                ctx.update(ActionResult::errorMessage("Action timed out"));
                if (ctx.canRetry())
                {
                    ctx.scheduleRetry();
                }
            }
            else if (invoker)
            {
                auto result = invoker(actor, ctx.token());
                ctx.update(std::move(result));

                if ((ctx.state() == ActionState::Failed || ctx.state() == ActionState::Error) && ctx.canRetry())
                {
                    ctx.scheduleRetry();
                }
            }
            break;

        case ActionState::Completed:
        case ActionState::Failed:
        case ActionState::Error:
        case ActionState::TimedOut:
        case ActionState::Cancelled:
            // Terminal states - nothing to do
            break;
    }
}

void ActionExecutor::cancel(ActionId id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = inFlight_.find(id);
    if (it != inFlight_.end())
    {
        it->second.context.cancel();
    }
}

void ActionExecutor::cancelAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, action] : inFlight_)
    {
        action.context.cancel();
    }
}

std::size_t ActionExecutor::inFlightCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return inFlight_.size();
}

bool ActionExecutor::hasInFlightActions() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return !inFlight_.empty();
}

} // namespace bnet::execution
