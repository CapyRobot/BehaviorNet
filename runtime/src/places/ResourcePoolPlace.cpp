// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/places/ResourcePoolPlace.hpp"

namespace bnet::places {

ResourcePoolPlace::ResourcePoolPlace(core::Place& place, std::size_t poolSize)
    : PlaceType(place)
    , poolSize_(poolSize)
{
    if (poolSize > 0)
    {
        initializePool(poolSize);
    }
}

void ResourcePoolPlace::onTokenEnter(Token /*token*/)
{
    // Resource returned to pool
}

void ResourcePoolPlace::tick(std::uint64_t /*epoch*/)
{
    // No periodic processing
}

void ResourcePoolPlace::initializePool(std::size_t count)
{
    poolSize_ = count;
    for (std::size_t i = 0; i < count; ++i)
    {
        place_.addToken(Token{});
    }
}

std::size_t ResourcePoolPlace::availableCount() const
{
    return place_.availableTokenCount();
}

std::optional<std::pair<core::TokenId, Token>> ResourcePoolPlace::acquire()
{
    return place_.removeToken();
}

void ResourcePoolPlace::release(Token token)
{
    place_.addToken(std::move(token));
}

} // namespace bnet::places
