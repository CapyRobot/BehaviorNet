// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PlaceType.hpp"

#include <functional>
#include <string>

namespace bnet::places {

/// @brief Callback for validating incoming tokens.
using TokenValidator = std::function<bool(const Token&)>;

/// @brief Place that accepts external token injection.
///
/// Entrypoint places are the starting points of a workflow.
/// External systems inject tokens here to trigger processing.
class EntrypointPlace : public PlaceType
{
public:
    explicit EntrypointPlace(core::Place& place);

    void onTokenEnter(Token token) override;
    void tick(std::uint64_t epoch) override;

    [[nodiscard]] std::string typeName() const override { return "EntrypointPlace"; }

    /// @brief Inject a token from external source.
    /// @return Token ID, or 0 if validation failed.
    core::TokenId inject(Token token);

    /// @brief Set validator for incoming tokens.
    void setValidator(TokenValidator validator) { validator_ = std::move(validator); }

    /// @brief Get count of tokens injected.
    [[nodiscard]] std::size_t injectedCount() const { return injectedCount_; }

private:
    TokenValidator validator_;
    std::size_t injectedCount_{0};
};

} // namespace bnet::places
