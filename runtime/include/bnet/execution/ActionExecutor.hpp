// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ActionContext.hpp"
#include "RetryPolicy.hpp"

#include <bnet/ActionResult.hpp>
#include <bnet/Actor.hpp>
#include <bnet/Token.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace bnet::execution {

/// @brief Function type for action invocation.
/// @param actor Pointer to actor (may be null for stateless actions).
/// @param token Reference to the token being processed.
using ActionInvoker = std::function<ActionResult(ActorBase*, Token&)>;

/// @brief Manages async action execution with retries and timeouts.
class ActionExecutor
{
public:
    ActionExecutor() = default;
    ~ActionExecutor() = default;

    ActionExecutor(const ActionExecutor&) = delete;
    ActionExecutor& operator=(const ActionExecutor&) = delete;
    ActionExecutor(ActionExecutor&&) = default;
    ActionExecutor& operator=(ActionExecutor&&) = default;

    /// @brief Start executing an action.
    /// @return Action ID for tracking.
    ActionId startAction(
        std::string actionName,
        Token token,
        ActorBase* actor,
        ActionInvoker invoker,
        RetryPolicy policy,
        ActionCallback callback);

    /// @brief Poll all in-flight actions, invoking callbacks on completion.
    void poll();

    /// @brief Cancel a specific action.
    void cancel(ActionId id);

    /// @brief Cancel all in-flight actions.
    void cancelAll();

    /// @brief Get number of in-flight actions.
    [[nodiscard]] std::size_t inFlightCount() const;

    /// @brief Check if any actions are in flight.
    [[nodiscard]] bool hasInFlightActions() const;

private:
    void processAction(ActionContext& ctx, ActorBase* actor, const ActionInvoker& invoker);

    struct InFlightAction
    {
        ActionContext context;
        ActorBase* actor;
        ActionInvoker invoker;
    };

    std::unordered_map<ActionId, InFlightAction> inFlight_;
    ActionId nextId_{1};
    mutable std::mutex mutex_;
};

} // namespace bnet::execution
