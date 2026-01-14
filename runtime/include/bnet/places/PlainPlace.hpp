// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PlaceType.hpp"

namespace bnet::places {

/// @brief Simple place with no special behavior.
///
/// Tokens enter and wait until consumed by a transition.
class PlainPlace : public PlaceType
{
public:
    using PlaceType::PlaceType;

    void onTokenEnter(Token /*token*/) override
    {
        // No special behavior
    }

    void tick(std::uint64_t /*epoch*/) override
    {
        // No periodic processing needed
    }

    [[nodiscard]] std::string typeName() const override
    {
        return "PlainPlace";
    }
};

} // namespace bnet::places
