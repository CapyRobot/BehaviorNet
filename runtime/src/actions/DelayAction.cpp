// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/actions/DelayAction.hpp"

namespace bnet::actions {

DelayAction::DelayAction(std::chrono::milliseconds defaultDelay)
    : defaultDelay_(defaultDelay)
{
}

ActionResult DelayAction::execute(Token& token)
{
    auto now = std::chrono::steady_clock::now();

    // Get or set start time
    if (!token.hasData("_delay_start"))
    {
        auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        token.setData("_delay_start", startMs);
    }

    // Get delay duration
    auto delayMs = defaultDelay_.count();
    if (token.hasData("delay_ms"))
    {
        delayMs = token.getData("delay_ms").get<int64_t>();
    }

    // Check if delay has elapsed
    auto startMs = token.getData("_delay_start").get<int64_t>();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() - startMs;

    if (elapsedMs >= delayMs)
    {
        // Clean up internal state
        token.data().erase("_delay_start");
        return ActionResult::success();
    }

    return ActionResult::inProgress();
}

FailAction::FailAction(std::string message)
    : message_(std::move(message))
{
}

ActionResult FailAction::execute(Token& token)
{
    token.setData("failure_message", message_);
    return ActionResult::failure(message_);
}

ErrorAction::ErrorAction(std::string message)
    : message_(std::move(message))
{
}

ActionResult ErrorAction::execute(Token& token)
{
    token.setData("error_message", message_);
    return ActionResult::errorMessage(message_);
}

} // namespace bnet::actions
