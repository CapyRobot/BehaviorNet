// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/core/Net.hpp"

#include <algorithm>
#include <stdexcept>

namespace bnet::core {

void Net::addPlace(std::unique_ptr<Place> place)
{
    std::string id = place->id();
    places_[id] = std::move(place);
}

void Net::addTransition(Transition transition)
{
    std::string id = transition.id();
    transitions_.emplace(id, std::move(transition));
}

void Net::addArc(Arc arc)
{
    arcs_.push_back(std::move(arc));
}

Place* Net::getPlace(std::string_view id)
{
    auto [placeId, sub] = parseSubplace(id);
    auto it = places_.find(placeId);
    return it != places_.end() ? it->second.get() : nullptr;
}

const Place* Net::getPlace(std::string_view id) const
{
    auto [placeId, sub] = parseSubplace(id);
    auto it = places_.find(placeId);
    return it != places_.end() ? it->second.get() : nullptr;
}

Transition* Net::getTransition(std::string_view id)
{
    auto it = transitions_.find(std::string(id));
    return it != transitions_.end() ? &it->second : nullptr;
}

const Transition* Net::getTransition(std::string_view id) const
{
    auto it = transitions_.find(std::string(id));
    return it != transitions_.end() ? &it->second : nullptr;
}

std::vector<Place*> Net::getAllPlaces()
{
    std::vector<Place*> result;
    result.reserve(places_.size());
    for (auto& [id, place] : places_)
    {
        result.push_back(place.get());
    }
    return result;
}

std::vector<const Place*> Net::getAllPlaces() const
{
    std::vector<const Place*> result;
    result.reserve(places_.size());
    for (const auto& [id, place] : places_)
    {
        result.push_back(place.get());
    }
    return result;
}

std::vector<Transition*> Net::getAllTransitions()
{
    std::vector<Transition*> result;
    result.reserve(transitions_.size());
    for (auto& [id, transition] : transitions_)
    {
        result.push_back(&transition);
    }
    return result;
}

std::vector<const Transition*> Net::getAllTransitions() const
{
    std::vector<const Transition*> result;
    result.reserve(transitions_.size());
    for (const auto& [id, transition] : transitions_)
    {
        result.push_back(&transition);
    }
    return result;
}

std::vector<Transition*> Net::getTransitionsByPriority()
{
    auto transitions = getAllTransitions();
    std::sort(transitions.begin(), transitions.end(),
        [](Transition* a, Transition* b)
        {
            if (a->priority() != b->priority())
            {
                return a->priority() > b->priority();  // Higher priority first
            }
            // Tie-breaker: transition fired less recently has higher priority
            return a->lastFiredEpoch() < b->lastFiredEpoch();
        });

    std::vector<Transition*> result;
    result.reserve(transitions.size());
    for (auto* t : transitions)
    {
        result.push_back(const_cast<Transition*>(t));
    }
    return result;
}

std::pair<Place*, Subplace> Net::resolvePlace(std::string_view placeRef)
{
    auto [placeId, sub] = parseSubplace(placeRef);
    auto it = places_.find(placeId);
    if (it == places_.end())
    {
        return {nullptr, sub};
    }
    return {it->second.get(), sub};
}

std::pair<const Place*, Subplace> Net::resolvePlace(std::string_view placeRef) const
{
    auto [placeId, sub] = parseSubplace(placeRef);
    auto it = places_.find(placeId);
    if (it == places_.end())
    {
        return {nullptr, sub};
    }
    return {it->second.get(), sub};
}

bool Net::isEnabled(const Transition& transition) const
{
    for (const auto& arc : transition.inputArcs())
    {
        auto [place, sub] = resolvePlace(arc.placeId());
        if (!place)
        {
            return false;
        }

        const TokenQueue* queue = nullptr;
        if (sub != Subplace::None && place->hasSubplaces())
        {
            queue = &place->subplace(sub);
        }
        else
        {
            queue = &place->tokens();
        }

        // Check if there's an available token matching the arc filter
        if (arc.tokenFilter().has_value())
        {
            // TODO: Check token has the required actor type
            // For now, just check availability
            if (queue->availableCount() < static_cast<std::size_t>(arc.weight()))
            {
                return false;
            }
        }
        else
        {
            if (queue->availableCount() < static_cast<std::size_t>(arc.weight()))
            {
                return false;
            }
        }
    }
    return true;
}

FireResult Net::fire(Transition& transition, std::uint64_t epoch)
{
    FireResult result;

    if (!isEnabled(transition))
    {
        result.errorMessage = "Transition not enabled: " + transition.id();
        return result;
    }

    // Consume tokens from input places
    for (const auto& arc : transition.inputArcs())
    {
        auto [place, sub] = resolvePlace(arc.placeId());
        if (!place)
        {
            result.errorMessage = "Input place not found: " + arc.placeId();
            return result;
        }

        TokenQueue* queue = nullptr;
        if (sub != Subplace::None && place->hasSubplaces())
        {
            queue = &place->subplace(sub);
        }
        else
        {
            queue = &place->tokens();
        }

        for (int i = 0; i < arc.weight(); ++i)
        {
            auto tokenPair = queue->pop();
            if (!tokenPair.has_value())
            {
                result.errorMessage = "Failed to consume token from: " + arc.placeId();
                return result;
            }
            result.consumedTokens.push_back(std::move(*tokenPair));
        }
    }

    // Produce tokens to output places
    // For now, we move consumed tokens to output places
    // In a real implementation, tokens might be merged/split based on arc configuration
    std::size_t tokenIdx = 0;
    for (const auto& arc : transition.outputArcs())
    {
        auto [place, sub] = resolvePlace(arc.placeId());
        if (!place)
        {
            result.errorMessage = "Output place not found: " + arc.placeId();
            return result;
        }

        TokenQueue* queue = nullptr;
        if (sub != Subplace::None && place->hasSubplaces())
        {
            queue = &place->subplace(sub);
        }
        else
        {
            queue = &place->tokens();
        }

        // Move tokens to output
        for (int i = 0; i < arc.weight() && tokenIdx < result.consumedTokens.size(); ++i)
        {
            queue->push(std::move(result.consumedTokens[tokenIdx].second));
            ++tokenIdx;
        }
    }

    transition.setLastFiredEpoch(epoch);
    result.success = true;
    return result;
}

std::vector<Transition*> Net::getEnabledTransitions()
{
    std::vector<Transition*> enabled;
    for (auto& [id, transition] : transitions_)
    {
        if (isEnabled(transition))
        {
            enabled.push_back(&transition);
        }
    }
    return enabled;
}

std::vector<const Arc*> Net::getArcsForTransition(std::string_view transitionId) const
{
    std::vector<const Arc*> result;
    for (const auto& arc : arcs_)
    {
        if (arc.transitionId() == transitionId)
        {
            result.push_back(&arc);
        }
    }
    return result;
}

std::vector<const Arc*> Net::getInputArcs(std::string_view placeId) const
{
    std::vector<const Arc*> result;
    auto [basePlaceId, sub] = parseSubplace(placeId);
    for (const auto& arc : arcs_)
    {
        if (arc.placeId() == basePlaceId &&
            arc.direction() == ArcDirection::TransitionToPlace)
        {
            result.push_back(&arc);
        }
    }
    return result;
}

std::vector<const Arc*> Net::getOutputArcs(std::string_view placeId) const
{
    std::vector<const Arc*> result;
    auto [basePlaceId, sub] = parseSubplace(placeId);
    for (const auto& arc : arcs_)
    {
        if (arc.placeId() == basePlaceId &&
            arc.direction() == ArcDirection::PlaceToTransition)
        {
            result.push_back(&arc);
        }
    }
    return result;
}

} // namespace bnet::core
