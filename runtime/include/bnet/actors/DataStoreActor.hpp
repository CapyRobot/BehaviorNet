// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/ActionResult.hpp>
#include <bnet/Actor.hpp>
#include <bnet/Token.hpp>

#include <nlohmann/json.hpp>

#include <mutex>
#include <string>
#include <unordered_map>

namespace bnet::actors {

/// @brief In-memory JSON key-value data store.
///
/// Provides set/get operations for storing arbitrary JSON data.
/// Can be used to pass data between actions or persist state.
class DataStoreActor : public ActorBase
{
public:
    DataStoreActor() = default;
    explicit DataStoreActor(const ActorParams& params);

    /// @brief Set a value by key.
    void set(const std::string& key, nlohmann::json value);

    /// @brief Get a value by key.
    [[nodiscard]] nlohmann::json get(const std::string& key) const;

    /// @brief Get a value with default if not found.
    [[nodiscard]] nlohmann::json getOr(const std::string& key, nlohmann::json defaultValue) const;

    /// @brief Check if key exists.
    [[nodiscard]] bool has(const std::string& key) const;

    /// @brief Remove a key.
    bool remove(const std::string& key);

    /// @brief Clear all data.
    void clear();

    /// @brief Get all keys.
    [[nodiscard]] std::vector<std::string> keys() const;

    /// @brief Get count of stored items.
    [[nodiscard]] std::size_t size() const;

    /// @brief Get entire store as JSON object.
    [[nodiscard]] nlohmann::json toJson() const;

    /// @brief Load from JSON object.
    void fromJson(const nlohmann::json& data);

    // Action methods (return ActionResult for use in nets)

    /// @brief Action: Set value from token data.
    /// Expects token to have "key" and "value" in data.
    ActionResult setValue(Token& token);

    /// @brief Action: Get value into token data.
    /// Expects token to have "key", stores result in "result".
    ActionResult getValue(Token& token);

    /// @brief Action: Check if key exists.
    /// Expects token to have "key", stores result in "exists".
    ActionResult hasKey(Token& token);

    /// @brief Action: Remove key.
    /// Expects token to have "key".
    ActionResult removeKey(Token& token);

private:
    std::unordered_map<std::string, nlohmann::json> store_;
    mutable std::mutex mutex_;
};

} // namespace bnet::actors
