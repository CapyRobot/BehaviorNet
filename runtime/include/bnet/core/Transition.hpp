// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Arc.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bnet::core {

/// @brief A transition in the Petri net that moves tokens between places.
class Transition
{
public:
    explicit Transition(std::string id);

    [[nodiscard]] const std::string& id() const { return id_; }

    void setPriority(int priority) { priority_ = priority; }
    [[nodiscard]] int priority() const { return priority_; }

    void addInputArc(Arc arc);
    void addOutputArc(Arc arc);

    [[nodiscard]] const std::vector<Arc>& inputArcs() const { return inputArcs_; }
    [[nodiscard]] const std::vector<Arc>& outputArcs() const { return outputArcs_; }

    void setLastFiredEpoch(std::uint64_t epoch) { lastFiredEpoch_ = epoch; }
    [[nodiscard]] std::uint64_t lastFiredEpoch() const { return lastFiredEpoch_; }

    /// @brief Check if this is an auto-triggering transition.
    ///
    /// AT transitions have no action-associated input places and fire
    /// automatically when enabled.
    void setAutoTrigger(bool autoTrigger) { autoTrigger_ = autoTrigger; }
    [[nodiscard]] bool isAutoTrigger() const { return autoTrigger_; }

private:
    std::string id_;
    int priority_{1};
    std::vector<Arc> inputArcs_;
    std::vector<Arc> outputArcs_;
    std::uint64_t lastFiredEpoch_{0};
    bool autoTrigger_{true};
};

} // namespace bnet::core
