// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PlaceType.hpp"

#include <functional>
#include <string>

namespace bnet::places {

/// @brief Place that manages a pool of resources.
///
/// Resource pools hold tokens representing available resources.
/// When a resource is acquired, it leaves the pool. When released, it returns.
/// The pool can be initialized with a fixed number of resource tokens.
class ResourcePoolPlace : public PlaceType
{
public:
    explicit ResourcePoolPlace(core::Place& place, std::size_t poolSize = 0);

    void onTokenEnter(Token token) override;
    void tick(std::uint64_t epoch) override;

    [[nodiscard]] std::string typeName() const override { return "ResourcePoolPlace"; }

    /// @brief Initialize pool with empty resource tokens.
    void initializePool(std::size_t count);

    /// @brief Get number of available resources.
    [[nodiscard]] std::size_t availableCount() const;

    /// @brief Get total pool size.
    [[nodiscard]] std::size_t poolSize() const { return poolSize_; }

    /// @brief Acquire a resource (remove from pool).
    /// @return Token if available, nullopt otherwise.
    [[nodiscard]] std::optional<std::pair<core::TokenId, Token>> acquire();

    /// @brief Release a resource back to pool.
    void release(Token token);

private:
    std::size_t poolSize_{0};
};

} // namespace bnet::places
