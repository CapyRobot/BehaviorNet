// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Token.hpp"

#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace bnet::core {

using TokenId = std::uint64_t;

/// @brief FIFO queue for tokens with waiting time tracking.
///
/// Tokens that have waited longer have higher priority for selection.
class TokenQueue
{
public:
    struct Entry
    {
        TokenId id;
        Token token;
        std::chrono::steady_clock::time_point arrivalTime;
        bool locked{false};  ///< Token is in use (action executing)
    };

    TokenQueue() = default;

    /// @brief Add a token to the queue.
    /// @return The assigned token ID.
    TokenId push(Token token);

    /// @brief Remove and return the highest-priority available token.
    [[nodiscard]] std::optional<std::pair<TokenId, Token>> pop();

    /// @brief Peek at the next available token without removing.
    [[nodiscard]] const Token* peek() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t availableCount() const;  ///< Not locked

    /// @brief Get token IDs sorted by waiting time (longest first).
    [[nodiscard]] std::vector<TokenId> getByWaitingTime() const;

    /// @brief Remove a specific token by ID.
    [[nodiscard]] std::optional<Token> remove(TokenId id);

    /// @brief Lock a token (mark as in-use for action execution).
    void lock(TokenId id);

    /// @brief Unlock a token.
    void unlock(TokenId id);

    /// @brief Check if queue has an available token matching actor requirements.
    [[nodiscard]] bool hasAvailableMatching(
        const std::function<bool(const Token&)>& predicate) const;

    /// @brief Get the first available token matching a predicate.
    [[nodiscard]] std::optional<TokenId> findAvailable(
        const std::function<bool(const Token&)>& predicate) const;

    /// @brief Access a token by ID (for inspection).
    [[nodiscard]] const Token* get(TokenId id) const;

private:
    std::deque<Entry> queue_;
    TokenId nextId_{1};
    mutable std::mutex mutex_;
};

} // namespace bnet::core
