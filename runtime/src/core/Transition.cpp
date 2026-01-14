// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/core/Transition.hpp"

namespace bnet::core {

Transition::Transition(std::string id)
    : id_(std::move(id))
{
}

void Transition::addInputArc(Arc arc)
{
    inputArcs_.push_back(std::move(arc));
}

void Transition::addOutputArc(Arc arc)
{
    outputArcs_.push_back(std::move(arc));
}

} // namespace bnet::core
