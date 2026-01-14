// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PlaceType.hpp"

#include <bnet/ActionResult.hpp>
#include <bnet/Actor.hpp>
#include <bnet/execution/ActionExecutor.hpp>
#include <bnet/execution/RetryPolicy.hpp>

#include <functional>
#include <string>

namespace bnet::places {

/// @brief Configuration for an action place.
struct ActionConfig
{
    std::string actorType;                      ///< Actor type ID (e.g., "user::Robot")
    std::string actionName;                     ///< Action method name
    execution::RetryPolicy retryPolicy;         ///< Retry configuration
};

/// @brief Place that executes an action on tokens.
///
/// When a token enters:
/// 1. Token moves to ::in_execution subplace
/// 2. Action is invoked on the token's actor
/// 3. Based on result, token moves to ::success, ::failure, or ::error
///
/// Requires the place to have subplaces enabled.
class ActionPlace : public PlaceType
{
public:
    ActionPlace(core::Place& place, ActionConfig config, execution::ActionExecutor& executor);

    void onTokenEnter(Token token) override;
    void tick(std::uint64_t epoch) override;

    [[nodiscard]] std::string typeName() const override { return "ActionPlace"; }

    /// @brief Set the action invoker function.
    void setInvoker(execution::ActionInvoker invoker) { invoker_ = std::move(invoker); }

    /// @brief Get the action configuration.
    [[nodiscard]] const ActionConfig& config() const { return config_; }

private:
    void onActionComplete(execution::ActionId id, ActionResult result, Token token);

    ActionConfig config_;
    execution::ActionExecutor& executor_;
    execution::ActionInvoker invoker_;
};

} // namespace bnet::places
