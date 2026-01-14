// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigTypes.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace bnet::config {

/// @brief Validation error from config parsing.
struct ValidationError
{
    std::string path;     ///< JSON path to error location
    std::string message;  ///< Error description
};

/// @brief Result of parsing a configuration.
struct ParseResult
{
    bool success{false};
    NetConfig config;
    std::vector<ValidationError> errors;
    std::vector<std::string> warnings;
};

/// @brief Parses BehaviorNet JSON configuration files.
class ConfigParser
{
public:
    ConfigParser() = default;

    /// @brief Parse configuration from JSON object.
    [[nodiscard]] ParseResult parse(const nlohmann::json& json);

    /// @brief Parse configuration from JSON string.
    [[nodiscard]] ParseResult parseString(const std::string& jsonStr);

    /// @brief Parse configuration from file.
    [[nodiscard]] ParseResult parseFile(const std::string& path);

private:
    ParseResult result_;

    void parseActors(const nlohmann::json& json);
    void parseActions(const nlohmann::json& json);
    void parsePlaces(const nlohmann::json& json);
    void parseTransitions(const nlohmann::json& json);

    PlaceConfig parsePlace(const nlohmann::json& json);
    TransitionConfig parseTransition(const nlohmann::json& json);
    OutputArc parseOutputArc(const nlohmann::json& json);

    PlaceType stringToPlaceType(const std::string& typeStr);
    PlaceParams parsePlaceParams(PlaceType type, const nlohmann::json& params);

    void addError(const std::string& path, const std::string& message);
    void addWarning(const std::string& message);
};

} // namespace bnet::config
