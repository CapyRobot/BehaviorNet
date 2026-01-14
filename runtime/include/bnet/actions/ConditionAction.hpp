// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/ActionResult.hpp>
#include <bnet/Token.hpp>

#include <functional>
#include <string>

namespace bnet::actions {

/// @brief Predicate function for condition checking.
using ConditionPredicate = std::function<bool(const Token&)>;

/// @brief Action that checks a boolean condition.
///
/// Returns SUCCESS if condition is true, FAILURE if false.
/// Useful for decision points in the workflow.
class ConditionAction
{
public:
    ConditionAction() = default;
    explicit ConditionAction(ConditionPredicate predicate);

    /// @brief Execute the condition check.
    /// If no predicate set, checks token data for "condition" key.
    ActionResult execute(Token& token);

    /// @brief Set the predicate function.
    void setPredicate(ConditionPredicate predicate) { predicate_ = std::move(predicate); }

    /// @brief Create a condition that checks a token data key for truthiness.
    static ConditionAction checkDataKey(const std::string& key);

    /// @brief Create a condition that checks if a token data key equals a value.
    static ConditionAction checkEquals(const std::string& key, nlohmann::json value);

    /// @brief Create a condition that checks if a token data key exists.
    static ConditionAction checkExists(const std::string& key);

    /// @brief Create a condition that checks a numeric comparison.
    static ConditionAction checkGreaterThan(const std::string& key, double value);
    static ConditionAction checkLessThan(const std::string& key, double value);

private:
    ConditionPredicate predicate_;
};

/// @brief Action that waits for a condition with timeout.
///
/// Returns IN_PROGRESS while waiting for condition.
/// Returns SUCCESS when condition becomes true.
/// Returns FAILURE when timeout is reached.
class WaitForConditionAction
{
public:
    WaitForConditionAction(ConditionPredicate condition, std::chrono::milliseconds timeout);

    ActionResult execute(Token& token);

    void setCondition(ConditionPredicate condition) { condition_ = std::move(condition); }
    void setTimeout(std::chrono::milliseconds timeout) { timeout_ = timeout; }

private:
    ConditionPredicate condition_;
    std::chrono::milliseconds timeout_;
};

} // namespace bnet::actions
