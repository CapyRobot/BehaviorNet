// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/places/ExitLoggerPlace.hpp"

namespace bnet::places {

ExitLoggerPlace::ExitLoggerPlace(core::Place& place)
    : PlaceType(place)
{
}

void ExitLoggerPlace::onTokenEnter(Token token)
{
    ++exitCount_;

    if (logger_)
    {
        logger_(place_.id(), token);
    }

    // Token is destroyed when it goes out of scope (not stored in place)
}

void ExitLoggerPlace::tick(std::uint64_t /*epoch*/)
{
    // Process any tokens that arrived via normal transitions
    while (place_.hasAvailableToken())
    {
        auto tokenPair = place_.removeToken();
        if (tokenPair.has_value())
        {
            ++exitCount_;
            if (logger_)
            {
                logger_(place_.id(), tokenPair->second);
            }
        }
    }
}

} // namespace bnet::places
