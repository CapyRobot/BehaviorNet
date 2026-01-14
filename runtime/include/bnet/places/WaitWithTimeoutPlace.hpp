// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PlaceType.hpp"

#include <chrono>
#include <functional>
#include <unordered_map>

namespace bnet::places {

/// @brief Callback to check if wait condition is satisfied.
using WaitCondition = std::function<bool(const Token&)>;

/// @brief Callback when wait times out.
using TimeoutCallback = std::function<void(Token&)>;

/// @brief Place that holds tokens until a condition or timeout.
///
/// Tokens wait in the main queue. On each tick:
/// - If condition is satisfied, token moves to ::success
/// - If timeout expires, token moves to ::failure
class WaitWithTimeoutPlace : public PlaceType
{
public:
    WaitWithTimeoutPlace(core::Place& place, std::chrono::milliseconds timeout);

    void onTokenEnter(Token token) override;
    void tick(std::uint64_t epoch) override;

    [[nodiscard]] std::string typeName() const override { return "WaitWithTimeoutPlace"; }

    /// @brief Set the wait condition.
    void setCondition(WaitCondition condition) { condition_ = std::move(condition); }

    /// @brief Set callback for timeouts.
    void setTimeoutCallback(TimeoutCallback callback) { timeoutCallback_ = std::move(callback); }

    /// @brief Get timeout duration.
    [[nodiscard]] std::chrono::milliseconds timeout() const { return timeout_; }

private:
    struct WaitEntry
    {
        std::chrono::steady_clock::time_point deadline;
    };

    std::chrono::milliseconds timeout_;
    WaitCondition condition_;
    TimeoutCallback timeoutCallback_;
    std::unordered_map<core::TokenId, WaitEntry> waitingTokens_;
};

} // namespace bnet::places
