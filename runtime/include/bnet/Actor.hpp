// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace bnet {

/// @brief Parameters passed to actor constructors from config.
class ActorParams
{
public:
    ActorParams() = default;
    explicit ActorParams(std::unordered_map<std::string, std::string> params)
        : params_(std::move(params)) {}

    /// @brief Get a required parameter.
    /// @throws std::runtime_error if not found.
    [[nodiscard]] const std::string& get(std::string_view key) const
    {
        auto it = params_.find(std::string(key));
        if (it == params_.end())
        {
            throw std::runtime_error(
                "Required actor parameter not found: " + std::string(key));
        }
        return it->second;
    }

    [[nodiscard]] std::string getOr(
        std::string_view key,
        std::string_view defaultValue) const
    {
        auto it = params_.find(std::string(key));
        return it == params_.end() ? std::string(defaultValue) : it->second;
    }

    [[nodiscard]] bool has(std::string_view key) const
    {
        return params_.find(std::string(key)) != params_.end();
    }

    [[nodiscard]] int getInt(std::string_view key) const
    {
        return std::stoi(get(key));
    }

    [[nodiscard]] int getIntOr(std::string_view key, int defaultValue) const
    {
        auto it = params_.find(std::string(key));
        if (it == params_.end()) return defaultValue;
        try
        {
            return std::stoi(it->second);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    [[nodiscard]] double getDouble(std::string_view key) const
    {
        return std::stod(get(key));
    }

    [[nodiscard]] double getDoubleOr(std::string_view key, double defaultValue) const
    {
        auto it = params_.find(std::string(key));
        if (it == params_.end()) return defaultValue;
        try
        {
            return std::stod(it->second);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    [[nodiscard]] bool getBool(std::string_view key) const
    {
        const auto& val = get(key);
        return val == "true" || val == "1" || val == "yes";
    }

    [[nodiscard]] bool getBoolOr(std::string_view key, bool defaultValue) const
    {
        auto it = params_.find(std::string(key));
        if (it == params_.end()) return defaultValue;
        const auto& val = it->second;
        return val == "true" || val == "1" || val == "yes";
    }

    void set(std::string_view key, std::string value)
    {
        params_[std::string(key)] = std::move(value);
    }

    [[nodiscard]] const std::unordered_map<std::string, std::string>& all() const
    {
        return params_;
    }

private:
    // TODO: should we use a json object instead?
    std::unordered_map<std::string, std::string> params_;
};

/// @brief Base class for all actors (domain entities like Vehicle, Robot, Charger).
class ActorBase
{
public:
    virtual ~ActorBase() = default;

    ActorBase(ActorBase&&) = default;
    ActorBase& operator=(ActorBase&&) = default;
    ActorBase(const ActorBase&) = delete;
    ActorBase& operator=(const ActorBase&) = delete;

protected:
    ActorBase() = default;
};

class Token;
class ActionResult;

using ActorFactory = std::function<std::unique_ptr<ActorBase>(const ActorParams&)>;
using ActionFunc = std::function<ActionResult(ActorBase&)>;
using ActionWithTokenFunc = std::function<ActionResult(ActorBase&, const Token&)>;

} // namespace bnet
