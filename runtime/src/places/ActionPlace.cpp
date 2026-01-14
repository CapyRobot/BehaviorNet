// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/places/ActionPlace.hpp"

namespace bnet::places {

ActionPlace::ActionPlace(core::Place& place, ActionConfig config, execution::ActionExecutor& executor)
    : PlaceType(place)
    , config_(std::move(config))
    , executor_(executor)
{
    // Ensure place has subplaces enabled for action result routing
    place_.enableSubplaces();
}

void ActionPlace::onTokenEnter(Token token)
{
    if (!invoker_)
    {
        // No invoker configured, move to error
        place_.subplace(core::Subplace::Error).push(std::move(token));
        return;
    }

    // Get the actor from the token
    // Note: In a full implementation, we'd look up the actor by type
    // For now, we assume the invoker handles actor lookup

    auto callback = [this](execution::ActionId id, ActionResult result, Token t)
    {
        onActionComplete(id, std::move(result), std::move(t));
    };

    // Start the action
    executor_.startAction(
        config_.actionName,
        std::move(token),
        nullptr,  // Actor passed via invoker
        invoker_,
        config_.retryPolicy,
        std::move(callback));
}

void ActionPlace::tick(std::uint64_t /*epoch*/)
{
    // ActionExecutor handles polling, nothing to do here
}

void ActionPlace::onActionComplete(execution::ActionId /*id*/, ActionResult result, Token token)
{
    switch (result.status())
    {
        case ActionResult::Status::Success:
            place_.subplace(core::Subplace::Success).push(std::move(token));
            break;
        case ActionResult::Status::Failure:
            place_.subplace(core::Subplace::Failure).push(std::move(token));
            break;
        case ActionResult::Status::Error:
            place_.subplace(core::Subplace::Error).push(std::move(token));
            break;
        case ActionResult::Status::InProgress:
            // Should not happen for completed actions
            place_.subplace(core::Subplace::Error).push(std::move(token));
            break;
    }
}

} // namespace bnet::places
