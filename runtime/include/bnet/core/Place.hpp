// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TokenQueue.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace bnet::core {

/// @brief Subplace identifiers for action places.
enum class Subplace
{
    None,
    Main,        ///< Main place (tokens waiting for action)
    InExecution, ///< Action currently executing
    Success,     ///< Action completed successfully
    Failure,     ///< Action failed after retries
    Error        ///< Action encountered error
};

/// @brief Parse a place reference that may include subplace.
///
/// Examples: "my_place" -> ("my_place", None)
///           "my_place::success" -> ("my_place", Success)
/// @return Pair of (place_id, subplace)
[[nodiscard]] std::pair<std::string, Subplace> parseSubplace(std::string_view placeRef);

/// @brief Convert subplace to string suffix.
[[nodiscard]] std::string_view subplaceToString(Subplace sub);

/// @brief A place in the Petri net that holds tokens.
class Place
{
public:
    explicit Place(std::string id);
    virtual ~Place() = default;

    Place(Place&&) = default;
    Place& operator=(Place&&) = default;
    Place(const Place&) = delete;
    Place& operator=(const Place&) = delete;

    [[nodiscard]] const std::string& id() const { return id_; }

    void setCapacity(std::size_t cap) { capacity_ = cap; }
    [[nodiscard]] std::optional<std::size_t> capacity() const { return capacity_; }

    void setRequiredActors(std::vector<std::string> actors);
    [[nodiscard]] const std::vector<std::string>& requiredActors() const
    {
        return requiredActors_;
    }

    /// @brief Add a token to the main queue.
    /// @return The token ID.
    /// @throws error::ResourceError if at capacity.
    TokenId addToken(Token token);

    /// @brief Remove and return the highest-priority available token.
    [[nodiscard]] std::optional<std::pair<TokenId, Token>> removeToken();

    /// @brief Remove a specific token by ID.
    [[nodiscard]] std::optional<Token> removeToken(TokenId id);

    /// @brief Check if place has available tokens.
    [[nodiscard]] bool hasAvailableToken() const;

    /// @brief Get count of all tokens (including locked).
    [[nodiscard]] std::size_t tokenCount() const;

    /// @brief Get count of available tokens.
    [[nodiscard]] std::size_t availableTokenCount() const;

    /// @brief Check if place can accept another token.
    [[nodiscard]] bool canAcceptToken() const;

    /// @brief Access the main token queue.
    [[nodiscard]] TokenQueue& tokens() { return tokens_; }
    [[nodiscard]] const TokenQueue& tokens() const { return tokens_; }

    // Subplace support for action places

    [[nodiscard]] bool hasSubplaces() const { return hasSubplaces_; }
    void enableSubplaces();

    /// @brief Access a subplace queue.
    /// @throws std::runtime_error if subplaces not enabled.
    [[nodiscard]] TokenQueue& subplace(Subplace sub);
    [[nodiscard]] const TokenQueue& subplace(Subplace sub) const;

    /// @brief Move a token from one subplace to another.
    void moveToken(TokenId id, Subplace from, Subplace to);

protected:
    std::string id_;
    std::optional<std::size_t> capacity_;
    std::vector<std::string> requiredActors_;
    TokenQueue tokens_;

    bool hasSubplaces_{false};
    std::unordered_map<Subplace, TokenQueue> subplaces_;
};

} // namespace bnet::core
