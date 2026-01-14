// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/actions/ConditionAction.hpp"

namespace bnet::actions {

ConditionAction::ConditionAction(ConditionPredicate predicate)
    : predicate_(std::move(predicate))
{
}

ActionResult ConditionAction::execute(Token& token)
{
    bool result = false;

    if (predicate_)
    {
        result = predicate_(token);
    }
    else if (token.hasData("condition"))
    {
        // Default: check "condition" key in token data
        const auto& condValue = token.getData("condition");
        if (condValue.is_boolean())
        {
            result = condValue.get<bool>();
        }
        else if (condValue.is_number())
        {
            result = condValue.get<double>() != 0;
        }
        else if (condValue.is_string())
        {
            const auto& str = condValue.get<std::string>();
            result = !str.empty() && str != "false" && str != "0";
        }
    }

    token.setData("condition_result", result);
    return result ? ActionResult::success() : ActionResult::failure("Condition not met");
}

ConditionAction ConditionAction::checkDataKey(const std::string& key)
{
    return ConditionAction([key](const Token& token) -> bool
    {
        if (!token.hasData(key))
        {
            return false;
        }
        const auto& value = token.getData(key);
        if (value.is_boolean())
        {
            return value.get<bool>();
        }
        if (value.is_number())
        {
            return value.get<double>() != 0;
        }
        if (value.is_string())
        {
            const auto& str = value.get<std::string>();
            return !str.empty() && str != "false" && str != "0";
        }
        return !value.is_null();
    });
}

ConditionAction ConditionAction::checkEquals(const std::string& key, nlohmann::json value)
{
    return ConditionAction([key, value = std::move(value)](const Token& token) -> bool
    {
        if (!token.hasData(key))
        {
            return false;
        }
        return token.getData(key) == value;
    });
}

ConditionAction ConditionAction::checkExists(const std::string& key)
{
    return ConditionAction([key](const Token& token) -> bool
    {
        return token.hasData(key);
    });
}

ConditionAction ConditionAction::checkGreaterThan(const std::string& key, double value)
{
    return ConditionAction([key, value](const Token& token) -> bool
    {
        if (!token.hasData(key))
        {
            return false;
        }
        try
        {
            return token.getData(key).get<double>() > value;
        }
        catch (...)
        {
            return false;
        }
    });
}

ConditionAction ConditionAction::checkLessThan(const std::string& key, double value)
{
    return ConditionAction([key, value](const Token& token) -> bool
    {
        if (!token.hasData(key))
        {
            return false;
        }
        try
        {
            return token.getData(key).get<double>() < value;
        }
        catch (...)
        {
            return false;
        }
    });
}

WaitForConditionAction::WaitForConditionAction(ConditionPredicate condition, std::chrono::milliseconds timeout)
    : condition_(std::move(condition))
    , timeout_(timeout)
{
}

ActionResult WaitForConditionAction::execute(Token& token)
{
    auto now = std::chrono::steady_clock::now();

    // Get or set start time
    if (!token.hasData("_wait_start"))
    {
        auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        token.setData("_wait_start", startMs);
    }

    // Check condition
    if (condition_ && condition_(token))
    {
        token.data().erase("_wait_start");
        return ActionResult::success();
    }

    // Check timeout
    auto startMs = token.getData("_wait_start").get<int64_t>();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() - startMs;

    if (elapsedMs >= timeout_.count())
    {
        token.data().erase("_wait_start");
        return ActionResult::failure("Wait timeout");
    }

    return ActionResult::inProgress();
}

} // namespace bnet::actions
