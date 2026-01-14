// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/places/EntrypointPlace.hpp"

namespace bnet::places {

EntrypointPlace::EntrypointPlace(core::Place& place)
    : PlaceType(place)
{
}

void EntrypointPlace::onTokenEnter(Token /*token*/)
{
    // Tokens enter via inject(), not through transitions
}

void EntrypointPlace::tick(std::uint64_t /*epoch*/)
{
    // No periodic processing
}

core::TokenId EntrypointPlace::inject(Token token)
{
    if (validator_ && !validator_(token))
    {
        return 0;
    }

    if (!place_.canAcceptToken())
    {
        return 0;
    }

    ++injectedCount_;
    return place_.addToken(std::move(token));
}

} // namespace bnet::places
