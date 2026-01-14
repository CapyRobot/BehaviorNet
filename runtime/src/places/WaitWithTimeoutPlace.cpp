// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/places/WaitWithTimeoutPlace.hpp"

namespace bnet::places {

WaitWithTimeoutPlace::WaitWithTimeoutPlace(core::Place& place, std::chrono::milliseconds timeout)
    : PlaceType(place)
    , timeout_(timeout)
{
    place_.enableSubplaces();
}

void WaitWithTimeoutPlace::onTokenEnter(Token token)
{
    auto deadline = std::chrono::steady_clock::now() + timeout_;
    auto id = place_.subplace(core::Subplace::Main).push(std::move(token));
    waitingTokens_[id] = WaitEntry{deadline};
}

void WaitWithTimeoutPlace::tick(std::uint64_t /*epoch*/)
{
    auto now = std::chrono::steady_clock::now();
    std::vector<core::TokenId> toRemove;

    // Check all waiting tokens
    for (auto& [id, entry] : waitingTokens_)
    {
        auto* tokenPtr = place_.subplace(core::Subplace::Main).get(id);
        if (!tokenPtr)
        {
            // Token no longer in queue (consumed by transition?)
            toRemove.push_back(id);
            continue;
        }

        // Check condition
        if (condition_ && condition_(*tokenPtr))
        {
            auto token = place_.subplace(core::Subplace::Main).remove(id);
            if (token.has_value())
            {
                place_.subplace(core::Subplace::Success).push(std::move(*token));
            }
            toRemove.push_back(id);
            continue;
        }

        // Check timeout
        if (now >= entry.deadline)
        {
            auto token = place_.subplace(core::Subplace::Main).remove(id);
            if (token.has_value())
            {
                if (timeoutCallback_)
                {
                    timeoutCallback_(*token);
                }
                place_.subplace(core::Subplace::Failure).push(std::move(*token));
            }
            toRemove.push_back(id);
        }
    }

    for (auto id : toRemove)
    {
        waitingTokens_.erase(id);
    }
}

} // namespace bnet::places
