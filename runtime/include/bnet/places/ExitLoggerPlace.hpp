// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PlaceType.hpp"

#include <functional>
#include <string>

namespace bnet::places {

/// @brief Callback for logging token exits.
using ExitLogger = std::function<void(const std::string& placeId, const Token& token)>;

/// @brief Place that logs and destroys tokens.
///
/// Exit places are terminal points in the workflow.
/// Tokens entering are logged and then destroyed.
class ExitLoggerPlace : public PlaceType
{
public:
    explicit ExitLoggerPlace(core::Place& place);

    void onTokenEnter(Token token) override;
    void tick(std::uint64_t epoch) override;

    [[nodiscard]] std::string typeName() const override { return "ExitLoggerPlace"; }

    /// @brief Set the logger callback.
    void setLogger(ExitLogger logger) { logger_ = std::move(logger); }

    /// @brief Get count of tokens that have exited.
    [[nodiscard]] std::size_t exitCount() const { return exitCount_; }

private:
    ExitLogger logger_;
    std::size_t exitCount_{0};
};

} // namespace bnet::places
