// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/core/TokenQueue.hpp"

#include <algorithm>

namespace bnet::core {

TokenId TokenQueue::push(Token token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    TokenId id = nextId_++;
    queue_.push_back(Entry{
        id,
        std::move(token),
        std::chrono::steady_clock::now(),
        false
    });
    return id;
}

std::optional<std::pair<TokenId, Token>> TokenQueue::pop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = queue_.begin(); it != queue_.end(); ++it)
    {
        if (!it->locked)
        {
            TokenId id = it->id;
            Token token = std::move(it->token);
            queue_.erase(it);
            return std::make_pair(id, std::move(token));
        }
    }
    return std::nullopt;
}

const Token* TokenQueue::peek() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : queue_)
    {
        if (!entry.locked)
        {
            return &entry.token;
        }
    }
    return nullptr;
}

bool TokenQueue::empty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

std::size_t TokenQueue::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

std::size_t TokenQueue::availableCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(queue_.begin(), queue_.end(),
        [](const Entry& e) { return !e.locked; });
}

std::vector<TokenId> TokenQueue::getByWaitingTime() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TokenId> ids;
    ids.reserve(queue_.size());
    for (const auto& entry : queue_)
    {
        if (!entry.locked)
        {
            ids.push_back(entry.id);
        }
    }
    // Already in FIFO order (oldest first)
    return ids;
}

std::optional<Token> TokenQueue::remove(TokenId id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = queue_.begin(); it != queue_.end(); ++it)
    {
        if (it->id == id)
        {
            Token token = std::move(it->token);
            queue_.erase(it);
            return token;
        }
    }
    return std::nullopt;
}

void TokenQueue::lock(TokenId id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& entry : queue_)
    {
        if (entry.id == id)
        {
            entry.locked = true;
            return;
        }
    }
}

void TokenQueue::unlock(TokenId id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& entry : queue_)
    {
        if (entry.id == id)
        {
            entry.locked = false;
            return;
        }
    }
}

bool TokenQueue::hasAvailableMatching(
    const std::function<bool(const Token&)>& predicate) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : queue_)
    {
        if (!entry.locked && predicate(entry.token))
        {
            return true;
        }
    }
    return false;
}

std::optional<TokenId> TokenQueue::findAvailable(
    const std::function<bool(const Token&)>& predicate) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : queue_)
    {
        if (!entry.locked && predicate(entry.token))
        {
            return entry.id;
        }
    }
    return std::nullopt;
}

const Token* TokenQueue::get(TokenId id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : queue_)
    {
        if (entry.id == id)
        {
            return &entry.token;
        }
    }
    return nullptr;
}

std::vector<std::pair<TokenId, nlohmann::json>> TokenQueue::getAllTokens() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<TokenId, nlohmann::json>> result;
    result.reserve(queue_.size());
    for (const auto& entry : queue_)
    {
        result.emplace_back(entry.id, entry.token.data());
    }
    return result;
}

} // namespace bnet::core
