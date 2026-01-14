// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/core/Arc.hpp"

namespace bnet::core {

Arc::Arc(
    std::string placeId,
    std::string transitionId,
    ArcDirection direction)
    : placeId_(std::move(placeId))
    , transitionId_(std::move(transitionId))
    , direction_(direction)
{
}

void Arc::setTokenFilter(std::string actorTypeId)
{
    tokenFilter_ = std::move(actorTypeId);
}

} // namespace bnet::core
