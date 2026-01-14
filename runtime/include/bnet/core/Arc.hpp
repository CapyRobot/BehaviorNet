// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

namespace bnet::core {

enum class ArcDirection
{
    PlaceToTransition,  ///< Input arc (place -> transition)
    TransitionToPlace   ///< Output arc (transition -> place)
};

/// @brief An arc connecting a place and a transition.
class Arc
{
public:
    Arc(
        std::string placeId,
        std::string transitionId,
        ArcDirection direction);

    [[nodiscard]] const std::string& placeId() const { return placeId_; }
    [[nodiscard]] const std::string& transitionId() const { return transitionId_; }
    [[nodiscard]] ArcDirection direction() const { return direction_; }

    void setTokenFilter(std::string actorTypeId);
    [[nodiscard]] const std::optional<std::string>& tokenFilter() const
    {
        return tokenFilter_;
    }

    void setWeight(int weight) { weight_ = weight; }
    [[nodiscard]] int weight() const { return weight_; }

private:
    std::string placeId_;
    std::string transitionId_;
    ArcDirection direction_;
    std::optional<std::string> tokenFilter_;
    int weight_{1};
};

} // namespace bnet::core
