// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/execution/ActionContext.hpp"

namespace bnet::execution {

ActionContext::ActionContext(ActionId id, std::string actionName, Token token, RetryPolicy policy, ActionCallback callback)
    : id_(id)
    , actionName_(std::move(actionName))
    , token_(std::move(token))
    , policy_(policy)
    , callback_(std::move(callback))
{
}

bool ActionContext::isTimedOut() const
{
    if (state_ != ActionState::Running)
    {
        return false;
    }
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    return elapsed >= policy_.timeout;
}

bool ActionContext::canRetry() const
{
    if (attemptCount_ >= policy_.maxRetries + 1)
    {
        return false;
    }
    if (state_ == ActionState::Error && policy_.retryOnError)
    {
        return true;
    }
    if (state_ == ActionState::Failed && policy_.retryOnFailure)
    {
        return true;
    }
    return false;
}

void ActionContext::start()
{
    state_ = ActionState::Running;
    startTime_ = std::chrono::steady_clock::now();
    ++attemptCount_;
}

void ActionContext::update(ActionResult result)
{
    lastResult_ = std::move(result);

    switch (lastResult_.status())
    {
        case ActionResult::Status::Success:
            state_ = ActionState::Completed;
            break;
        case ActionResult::Status::Failure:
            state_ = ActionState::Failed;
            break;
        case ActionResult::Status::Error:
            state_ = ActionState::Error;
            break;
        case ActionResult::Status::InProgress:
            // Still running
            break;
    }
}

void ActionContext::scheduleRetry()
{
    state_ = ActionState::Pending;
    retryTime_ = std::chrono::steady_clock::now() + policy_.retryDelay;
}

bool ActionContext::isReadyForRetry() const
{
    if (state_ != ActionState::Pending)
    {
        return false;
    }
    return std::chrono::steady_clock::now() >= retryTime_;
}

void ActionContext::cancel()
{
    state_ = ActionState::Cancelled;
}

void ActionContext::invokeCallback()
{
    if (callbackInvoked_ || !callback_)
    {
        return;
    }
    callbackInvoked_ = true;
    callback_(id_, std::move(lastResult_), std::move(token_));
}

} // namespace bnet::execution
