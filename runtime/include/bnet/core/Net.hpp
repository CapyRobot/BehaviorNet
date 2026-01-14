// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Arc.hpp"
#include "Place.hpp"
#include "Transition.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace bnet::core {

/// @brief Result of firing a transition.
struct FireResult
{
    bool success{false};
    std::vector<std::pair<TokenId, Token>> consumedTokens;
    std::string errorMessage;
};

/// @brief Container for the complete Petri net structure.
class Net
{
public:
    Net() = default;

    void addPlace(std::unique_ptr<Place> place);
    void addTransition(Transition transition);
    void addArc(Arc arc);

    [[nodiscard]] Place* getPlace(std::string_view id);
    [[nodiscard]] const Place* getPlace(std::string_view id) const;

    [[nodiscard]] Transition* getTransition(std::string_view id);
    [[nodiscard]] const Transition* getTransition(std::string_view id) const;

    [[nodiscard]] std::vector<Place*> getAllPlaces();
    [[nodiscard]] std::vector<const Place*> getAllPlaces() const;

    [[nodiscard]] std::vector<Transition*> getAllTransitions();
    [[nodiscard]] std::vector<const Transition*> getAllTransitions() const;

    /// @brief Get transitions sorted by priority (highest first, then by last fired).
    [[nodiscard]] std::vector<Transition*> getTransitionsByPriority();

    /// @brief Check if a transition is enabled.
    ///
    /// A transition is enabled when all input places have available tokens
    /// that match any arc filters.
    [[nodiscard]] bool isEnabled(const Transition& transition) const;

    /// @brief Fire a transition, moving tokens from inputs to outputs.
    /// @param transition The transition to fire.
    /// @param epoch Current execution epoch (for tie-breaking).
    /// @return Result including consumed tokens.
    [[nodiscard]] FireResult fire(Transition& transition, std::uint64_t epoch);

    /// @brief Get all enabled transitions.
    [[nodiscard]] std::vector<Transition*> getEnabledTransitions();

    /// @brief Get arcs for a transition.
    [[nodiscard]] std::vector<const Arc*> getArcsForTransition(
        std::string_view transitionId) const;

    /// @brief Get input arcs for a place.
    [[nodiscard]] std::vector<const Arc*> getInputArcs(std::string_view placeId) const;

    /// @brief Get output arcs for a place.
    [[nodiscard]] std::vector<const Arc*> getOutputArcs(std::string_view placeId) const;

    /// @brief Resolve place reference (handles subplaces).
    [[nodiscard]] std::pair<Place*, Subplace> resolvePlace(std::string_view placeRef);
    [[nodiscard]] std::pair<const Place*, Subplace> resolvePlace(
        std::string_view placeRef) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Place>> places_;
    std::unordered_map<std::string, Transition> transitions_;
    std::vector<Arc> arcs_;
};

} // namespace bnet::core
