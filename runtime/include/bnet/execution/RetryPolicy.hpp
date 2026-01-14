// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>

namespace bnet::execution {

/// @brief Configuration for action retry behavior.
struct RetryPolicy
{
    std::uint32_t maxRetries{3};                                 ///< Maximum retry attempts (0 = no retries)
    std::chrono::milliseconds timeout{30000};                    ///< Action timeout
    std::chrono::milliseconds retryDelay{1000};                  ///< Delay between retries
    bool retryOnError{true};                                     ///< Retry on ERROR status
    bool retryOnFailure{false};                                  ///< Retry on FAILURE status

    /// @brief Create policy with no retries.
    [[nodiscard]] static RetryPolicy noRetry()
    {
        return RetryPolicy{0, std::chrono::milliseconds{30000}, std::chrono::milliseconds{0}, false, false};
    }

    /// @brief Create policy with immediate retries.
    [[nodiscard]] static RetryPolicy immediate(std::uint32_t maxRetries)
    {
        return RetryPolicy{maxRetries, std::chrono::milliseconds{30000}, std::chrono::milliseconds{0}, true, false};
    }
};

} // namespace bnet::execution
