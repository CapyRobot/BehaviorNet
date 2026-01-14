// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/ActionResult.hpp>
#include <bnet/Token.hpp>

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace bnet::actions {

/// @brief Action that delays for a specified duration.
///
/// Returns IN_PROGRESS until the delay has elapsed, then returns SUCCESS.
/// The delay duration can be specified in the constructor or per-token via data.
class DelayAction
{
public:
    explicit DelayAction(std::chrono::milliseconds defaultDelay = std::chrono::milliseconds(1000));

    /// @brief Execute the delay action.
    /// Token data: delay_ms (optional, uses default if not set)
    /// Stores _delay_start in token data to track start time.
    ActionResult execute(Token& token);

    /// @brief Set default delay duration.
    void setDefaultDelay(std::chrono::milliseconds delay) { defaultDelay_ = delay; }

    /// @brief Get default delay duration.
    [[nodiscard]] std::chrono::milliseconds defaultDelay() const { return defaultDelay_; }

private:
    std::chrono::milliseconds defaultDelay_;
};

/// @brief Action that always returns SUCCESS (no-op).
class NoOpAction
{
public:
    ActionResult execute(Token& /*token*/) { return ActionResult::success(); }
};

/// @brief Action that always returns FAILURE.
class FailAction
{
public:
    explicit FailAction(std::string message = "Intentional failure");

    ActionResult execute(Token& token);

private:
    std::string message_;
};

/// @brief Action that always returns ERROR.
class ErrorAction
{
public:
    explicit ErrorAction(std::string message = "Intentional error");

    ActionResult execute(Token& token);

private:
    std::string message_;
};

} // namespace bnet::actions
