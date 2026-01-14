// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

namespace bnet::config {

/// @brief Parameter type specification.
struct ParamSpec
{
    std::string type;  ///< "str", "int", "float", "bool"
};

/// @brief Actor type definition from config.
struct ActorConfig
{
    std::string id;
    std::map<std::string, ParamSpec> requiredInitParams;
    std::map<std::string, ParamSpec> optionalInitParams;
};

/// @brief Action definition from config.
struct ActionConfig
{
    std::string id;
    std::vector<std::string> requiredActors;
};

/// @brief Parameters for entrypoint place type.
struct EntrypointParams
{
    std::vector<std::string> newActors;
};

/// @brief Parameters for resource pool place type.
struct ResourcePoolParams
{
    std::string resourceId;
    int initialAvailability{0};
};

/// @brief Parameters for wait with timeout place type.
struct WaitWithTimeoutParams
{
    std::chrono::seconds timeout{60};
    std::string onTimeout;
};

/// @brief Parameters for action place type.
struct ActionPlaceParams
{
    std::string actionId;
    int retries{0};
    std::chrono::seconds timeoutPerTry{30};
    bool failureAsError{false};
    bool errorToGlobalHandler{true};
};

/// @brief Parameters for exit logger place type.
struct ExitLoggerParams
{
    // No specific params
};

/// @brief Parameters for plain place type.
struct PlainParams
{
    // No specific params
};

using PlaceParams = std::variant<
    PlainParams,
    EntrypointParams,
    ResourcePoolParams,
    WaitWithTimeoutParams,
    ActionPlaceParams,
    ExitLoggerParams>;

/// @brief Place type enumeration.
enum class PlaceType
{
    Plain,
    Entrypoint,
    ResourcePool,
    WaitWithTimeout,
    Action,
    ExitLogger
};

/// @brief Place definition from config.
struct PlaceConfig
{
    std::string id;
    PlaceType type{PlaceType::Plain};
    PlaceParams params;
};

/// @brief Output arc configuration (can specify token filter).
struct OutputArc
{
    std::string to;
    std::optional<std::string> tokenFilter;
};

/// @brief Transition definition from config.
struct TransitionConfig
{
    std::vector<std::string> from;        ///< Input place IDs (may include subplace suffix)
    std::vector<OutputArc> to;            ///< Output arcs
    std::optional<int> priority;          ///< Optional priority (higher = preferred)
};

/// @brief Complete BehaviorNet configuration.
struct NetConfig
{
    std::vector<ActorConfig> actors;
    std::vector<ActionConfig> actions;
    std::vector<PlaceConfig> places;
    std::vector<TransitionConfig> transitions;
    nlohmann::json guiMetadata;           ///< Optional GUI layout data
};

} // namespace bnet::config
