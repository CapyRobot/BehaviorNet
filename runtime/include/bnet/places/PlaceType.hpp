// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/Token.hpp>
#include <bnet/core/Place.hpp>
#include <bnet/core/TokenQueue.hpp>

#include <functional>
#include <string>

namespace bnet::places {

/// @brief Callback for when a token enters a place.
using TokenEntryCallback = std::function<void(const std::string& placeId, Token& token)>;

/// @brief Callback for when a token exits a place.
using TokenExitCallback = std::function<void(const std::string& placeId, core::Subplace subplace, Token& token)>;

/// @brief Base interface for specialized place behaviors.
///
/// Place types add behavior on top of basic token storage:
/// - ActionPlace executes an action and routes to subplaces
/// - EntrypointPlace accepts external token injection
/// - ResourcePoolPlace manages resource allocation
/// - WaitWithTimeoutPlace holds tokens for a duration
/// - ExitLoggerPlace logs and destroys tokens
class PlaceType
{
public:
    explicit PlaceType(core::Place& place) : place_(place) {}
    virtual ~PlaceType() = default;

    PlaceType(const PlaceType&) = delete;
    PlaceType& operator=(const PlaceType&) = delete;
    PlaceType(PlaceType&&) = default;
    PlaceType& operator=(PlaceType&&) = default;

    /// @brief Called when a token enters the place.
    virtual void onTokenEnter(Token token) = 0;

    /// @brief Called periodically to process tokens (e.g., check timeouts).
    virtual void tick(std::uint64_t epoch) = 0;

    /// @brief Get the type name for debugging/logging.
    [[nodiscard]] virtual std::string typeName() const = 0;

    /// @brief Access the underlying place.
    [[nodiscard]] core::Place& place() { return place_; }
    [[nodiscard]] const core::Place& place() const { return place_; }

protected:
    core::Place& place_;
};

} // namespace bnet::places
