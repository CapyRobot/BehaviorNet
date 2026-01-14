// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Error.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace bnet {

class ActorBase;

/// @brief A collection of actors flowing through the net.
///
/// Tokens move between places as transitions fire. Each token contains
/// zero or more actors (domain entities like Vehicle, Robot, Charger).
class Token
{
public:
    Token() = default;
    ~Token() = default;

    Token(Token&&) = default;
    Token& operator=(Token&&) = default;
    Token(const Token&) = delete;
    Token& operator=(const Token&) = delete;

    /// @brief Get an actor of the specified type.
    /// @throws error::ActorNotFoundError if not present.
    template <typename T>
    [[nodiscard]] const T& getActor() const
    {
        auto it = actors_.find(std::type_index(typeid(T)));
        if (it == actors_.end())
        {
            throw error::ActorNotFoundError(typeid(T).name());
        }
        return *static_cast<const T*>(it->second.get());
    }

    /// @brief Add an actor to this token (takes ownership, replaces existing).
    template <typename T>
    void addActor(std::unique_ptr<T> actor)
    {
        static_assert(std::is_base_of_v<ActorBase, T>,
                      "Actor must derive from ActorBase");
        actors_[std::type_index(typeid(T))] = std::move(actor);
    }

    /// @brief Remove an actor of the specified type.
    /// @return The removed actor, or nullptr if not found.
    template <typename T>
    std::unique_ptr<T> removeActor()
    {
        auto it = actors_.find(std::type_index(typeid(T)));
        if (it == actors_.end())
        {
            return nullptr;
        }
        auto actor = std::unique_ptr<T>(static_cast<T*>(it->second.release()));
        actors_.erase(it);
        return actor;
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<ActorBase>> actors_;
};

} // namespace bnet
