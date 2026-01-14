// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/core/Place.hpp"
#include "bnet/Error.hpp"

#include <stdexcept>

namespace bnet::core {

std::pair<std::string, Subplace> parseSubplace(std::string_view placeRef)
{
    auto pos = placeRef.find("::");
    if (pos == std::string_view::npos)
    {
        return {std::string(placeRef), Subplace::None};
    }

    std::string placeId(placeRef.substr(0, pos));
    std::string_view suffix = placeRef.substr(pos + 2);

    Subplace sub = Subplace::None;
    if (suffix == "main")
    {
        sub = Subplace::Main;
    }
    else if (suffix == "in_execution")
    {
        sub = Subplace::InExecution;
    }
    else if (suffix == "success")
    {
        sub = Subplace::Success;
    }
    else if (suffix == "failure")
    {
        sub = Subplace::Failure;
    }
    else if (suffix == "error")
    {
        sub = Subplace::Error;
    }

    return {placeId, sub};
}

std::string_view subplaceToString(Subplace sub)
{
    switch (sub)
    {
        case Subplace::None: return "";
        case Subplace::Main: return "main";
        case Subplace::InExecution: return "in_execution";
        case Subplace::Success: return "success";
        case Subplace::Failure: return "failure";
        case Subplace::Error: return "error";
    }
    return "";
}

Place::Place(std::string id)
    : id_(std::move(id))
{
}

void Place::setRequiredActors(std::vector<std::string> actors)
{
    requiredActors_ = std::move(actors);
}

TokenId Place::addToken(Token token)
{
    if (capacity_.has_value() && tokens_.size() >= *capacity_)
    {
        throw error::ResourceError("Place at capacity: " + id_);
    }
    return tokens_.push(std::move(token));
}

std::optional<std::pair<TokenId, Token>> Place::removeToken()
{
    return tokens_.pop();
}

std::optional<Token> Place::removeToken(TokenId id)
{
    return tokens_.remove(id);
}

bool Place::hasAvailableToken() const
{
    return tokens_.availableCount() > 0;
}

std::size_t Place::tokenCount() const
{
    std::size_t count = tokens_.size();
    if (hasSubplaces_)
    {
        for (const auto& [sub, queue] : subplaces_)
        {
            count += queue.size();
        }
    }
    return count;
}

std::size_t Place::availableTokenCount() const
{
    return tokens_.availableCount();
}

bool Place::canAcceptToken() const
{
    if (!capacity_.has_value())
    {
        return true;
    }
    return tokens_.size() < *capacity_;
}

void Place::enableSubplaces()
{
    if (hasSubplaces_)
    {
        return;
    }
    hasSubplaces_ = true;
    subplaces_.try_emplace(Subplace::Main);
    subplaces_.try_emplace(Subplace::InExecution);
    subplaces_.try_emplace(Subplace::Success);
    subplaces_.try_emplace(Subplace::Failure);
    subplaces_.try_emplace(Subplace::Error);
}

TokenQueue& Place::subplace(Subplace sub)
{
    if (!hasSubplaces_)
    {
        throw std::runtime_error("Subplaces not enabled for place: " + id_);
    }
    if (sub == Subplace::None)
    {
        return tokens_;
    }
    return subplaces_.at(sub);
}

const TokenQueue& Place::subplace(Subplace sub) const
{
    if (!hasSubplaces_)
    {
        throw std::runtime_error("Subplaces not enabled for place: " + id_);
    }
    if (sub == Subplace::None)
    {
        return tokens_;
    }
    return subplaces_.at(sub);
}

void Place::moveToken(TokenId id, Subplace from, Subplace to)
{
    auto& fromQueue = (from == Subplace::None) ? tokens_ : subplaces_.at(from);
    auto& toQueue = (to == Subplace::None) ? tokens_ : subplaces_.at(to);

    auto token = fromQueue.remove(id);
    if (token.has_value())
    {
        toQueue.push(std::move(*token));
    }
}

} // namespace bnet::core
