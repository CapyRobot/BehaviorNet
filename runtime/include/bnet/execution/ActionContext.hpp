// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "RetryPolicy.hpp"

#include <bnet/ActionResult.hpp>
#include <bnet/Token.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace bnet::execution {

using ActionId = std::uint64_t;

/// @brief Callback invoked when action completes (success, failure, or error).
using ActionCallback = std::function<void(ActionId, ActionResult, Token)>;

/// @brief State of an in-flight action.
enum class ActionState
{
    Pending,      ///< Waiting to start
    Running,      ///< Currently executing
    Completed,    ///< Finished successfully
    Failed,       ///< Finished with failure
    Error,        ///< Finished with error
    TimedOut,     ///< Exceeded timeout
    Cancelled     ///< Manually cancelled
};

/// @brief Context for tracking an action's execution.
class ActionContext
{
public:
    ActionContext(ActionId id, std::string actionName, Token token, RetryPolicy policy, ActionCallback callback);

    [[nodiscard]] ActionId id() const { return id_; }
    [[nodiscard]] const std::string& actionName() const { return actionName_; }
    [[nodiscard]] ActionState state() const { return state_; }
    [[nodiscard]] std::uint32_t attemptCount() const { return attemptCount_; }
    [[nodiscard]] const RetryPolicy& policy() const { return policy_; }
    [[nodiscard]] Token& token() { return token_; }
    [[nodiscard]] const Token& token() const { return token_; }

    /// @brief Check if timeout has been exceeded.
    [[nodiscard]] bool isTimedOut() const;

    /// @brief Check if retry is allowed.
    [[nodiscard]] bool canRetry() const;

    /// @brief Mark action as started.
    void start();

    /// @brief Update with action result.
    void update(ActionResult result);

    /// @brief Schedule a retry attempt.
    void scheduleRetry();

    /// @brief Check if ready for retry (delay passed).
    [[nodiscard]] bool isReadyForRetry() const;

    /// @brief Mark as cancelled.
    void cancel();

    /// @brief Invoke completion callback with current state.
    void invokeCallback();

    /// @brief Get the last action result.
    [[nodiscard]] const ActionResult& lastResult() const { return lastResult_; }

private:
    ActionId id_;
    std::string actionName_;
    Token token_;
    RetryPolicy policy_;
    ActionCallback callback_;
    ActionState state_{ActionState::Pending};
    ActionResult lastResult_;
    std::uint32_t attemptCount_{0};
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point retryTime_;
    bool callbackInvoked_{false};
};

} // namespace bnet::execution
