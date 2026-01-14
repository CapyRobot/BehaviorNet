// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/config/ConfigParser.hpp"

#include <fstream>
#include <sstream>

namespace bnet::config {

ParseResult ConfigParser::parse(const nlohmann::json& json)
{
    result_ = ParseResult{};

    try
    {
        if (json.contains("actors"))
        {
            parseActors(json["actors"]);
        }

        if (json.contains("actions"))
        {
            parseActions(json["actions"]);
        }

        if (json.contains("places"))
        {
            parsePlaces(json["places"]);
        }
        else
        {
            addError("", "Missing required 'places' array");
        }

        if (json.contains("transitions"))
        {
            parseTransitions(json["transitions"]);
        }
        else
        {
            addError("", "Missing required 'transitions' array");
        }

        if (json.contains("_gui_metadata"))
        {
            result_.config.guiMetadata = json["_gui_metadata"];
        }

        result_.success = result_.errors.empty();
    }
    catch (const std::exception& e)
    {
        addError("", std::string("Parse error: ") + e.what());
    }

    return std::move(result_);
}

ParseResult ConfigParser::parseString(const std::string& jsonStr)
{
    try
    {
        auto json = nlohmann::json::parse(jsonStr);
        return parse(json);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        ParseResult result;
        result.errors.push_back({"", std::string("JSON parse error: ") + e.what()});
        return result;
    }
}

ParseResult ConfigParser::parseFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        ParseResult result;
        result.errors.push_back({"", "Failed to open file: " + path});
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

void ConfigParser::parseActors(const nlohmann::json& json)
{
    if (!json.is_array())
    {
        addError("actors", "Expected array");
        return;
    }

    for (std::size_t i = 0; i < json.size(); ++i)
    {
        const auto& item = json[i];
        ActorConfig actor;

        if (!item.contains("id") || !item["id"].is_string())
        {
            addError("actors[" + std::to_string(i) + "]", "Missing or invalid 'id'");
            continue;
        }
        actor.id = item["id"].get<std::string>();

        if (item.contains("required_init_params") && item["required_init_params"].is_object())
        {
            for (auto& [key, value] : item["required_init_params"].items())
            {
                ParamSpec spec;
                if (value.contains("type"))
                {
                    spec.type = value["type"].get<std::string>();
                }
                actor.requiredInitParams[key] = spec;
            }
        }

        if (item.contains("optional_init_params") && item["optional_init_params"].is_object())
        {
            for (auto& [key, value] : item["optional_init_params"].items())
            {
                ParamSpec spec;
                if (value.contains("type"))
                {
                    spec.type = value["type"].get<std::string>();
                }
                actor.optionalInitParams[key] = spec;
            }
        }

        result_.config.actors.push_back(std::move(actor));
    }
}

void ConfigParser::parseActions(const nlohmann::json& json)
{
    if (!json.is_array())
    {
        addError("actions", "Expected array");
        return;
    }

    for (std::size_t i = 0; i < json.size(); ++i)
    {
        const auto& item = json[i];
        ActionConfig action;

        if (!item.contains("id") || !item["id"].is_string())
        {
            addError("actions[" + std::to_string(i) + "]", "Missing or invalid 'id'");
            continue;
        }
        action.id = item["id"].get<std::string>();

        if (item.contains("required_actors") && item["required_actors"].is_array())
        {
            for (const auto& actor : item["required_actors"])
            {
                if (actor.is_string())
                {
                    action.requiredActors.push_back(actor.get<std::string>());
                }
            }
        }

        result_.config.actions.push_back(std::move(action));
    }
}

void ConfigParser::parsePlaces(const nlohmann::json& json)
{
    if (!json.is_array())
    {
        addError("places", "Expected array");
        return;
    }

    for (std::size_t i = 0; i < json.size(); ++i)
    {
        try
        {
            result_.config.places.push_back(parsePlace(json[i]));
        }
        catch (const std::exception& e)
        {
            addError("places[" + std::to_string(i) + "]", e.what());
        }
    }
}

PlaceConfig ConfigParser::parsePlace(const nlohmann::json& json)
{
    PlaceConfig place;

    if (!json.contains("id") || !json["id"].is_string())
    {
        throw std::runtime_error("Missing or invalid 'id'");
    }
    place.id = json["id"].get<std::string>();

    std::string typeStr = "plain";
    if (json.contains("type") && json["type"].is_string())
    {
        typeStr = json["type"].get<std::string>();
    }
    place.type = stringToPlaceType(typeStr);

    nlohmann::json params = nlohmann::json::object();
    if (json.contains("params"))
    {
        params = json["params"];
    }
    place.params = parsePlaceParams(place.type, params);

    return place;
}

PlaceType ConfigParser::stringToPlaceType(const std::string& typeStr)
{
    if (typeStr == "entrypoint") return PlaceType::Entrypoint;
    if (typeStr == "resource_pool") return PlaceType::ResourcePool;
    if (typeStr == "wait_with_timeout") return PlaceType::WaitWithTimeout;
    if (typeStr == "action") return PlaceType::Action;
    if (typeStr == "exit_logger") return PlaceType::ExitLogger;
    return PlaceType::Plain;
}

PlaceParams ConfigParser::parsePlaceParams(PlaceType type, const nlohmann::json& params)
{
    switch (type)
    {
        case PlaceType::Entrypoint:
        {
            EntrypointParams p;
            if (params.contains("new_actors") && params["new_actors"].is_array())
            {
                for (const auto& actor : params["new_actors"])
                {
                    if (actor.is_string())
                    {
                        p.newActors.push_back(actor.get<std::string>());
                    }
                }
            }
            return p;
        }

        case PlaceType::ResourcePool:
        {
            ResourcePoolParams p;
            if (params.contains("resource_id"))
            {
                p.resourceId = params["resource_id"].get<std::string>();
            }
            if (params.contains("initial_availability"))
            {
                p.initialAvailability = params["initial_availability"].get<int>();
            }
            return p;
        }

        case PlaceType::WaitWithTimeout:
        {
            WaitWithTimeoutParams p;
            if (params.contains("timeout_min"))
            {
                p.timeout = std::chrono::minutes(params["timeout_min"].get<int>());
            }
            else if (params.contains("timeout_s"))
            {
                p.timeout = std::chrono::seconds(params["timeout_s"].get<int>());
            }
            if (params.contains("on_timeout"))
            {
                p.onTimeout = params["on_timeout"].get<std::string>();
            }
            return p;
        }

        case PlaceType::Action:
        {
            ActionPlaceParams p;
            if (params.contains("action_id"))
            {
                p.actionId = params["action_id"].get<std::string>();
            }
            if (params.contains("retries"))
            {
                p.retries = params["retries"].get<int>();
            }
            if (params.contains("timeout_per_try_min"))
            {
                p.timeoutPerTry = std::chrono::minutes(params["timeout_per_try_min"].get<int>());
            }
            else if (params.contains("timeout_per_try_s"))
            {
                p.timeoutPerTry = std::chrono::seconds(params["timeout_per_try_s"].get<int>());
            }
            if (params.contains("failure_as_error"))
            {
                p.failureAsError = params["failure_as_error"].get<bool>();
            }
            if (params.contains("error_to_global_handler"))
            {
                p.errorToGlobalHandler = params["error_to_global_handler"].get<bool>();
            }
            return p;
        }

        case PlaceType::ExitLogger:
            return ExitLoggerParams{};

        case PlaceType::Plain:
        default:
            return PlainParams{};
    }
}

void ConfigParser::parseTransitions(const nlohmann::json& json)
{
    if (!json.is_array())
    {
        addError("transitions", "Expected array");
        return;
    }

    for (std::size_t i = 0; i < json.size(); ++i)
    {
        try
        {
            result_.config.transitions.push_back(parseTransition(json[i]));
        }
        catch (const std::exception& e)
        {
            addError("transitions[" + std::to_string(i) + "]", e.what());
        }
    }
}

TransitionConfig ConfigParser::parseTransition(const nlohmann::json& json)
{
    TransitionConfig trans;

    if (!json.contains("from") || !json["from"].is_array())
    {
        throw std::runtime_error("Missing or invalid 'from' array");
    }

    for (const auto& item : json["from"])
    {
        if (item.is_string())
        {
            trans.from.push_back(item.get<std::string>());
        }
    }

    if (!json.contains("to") || !json["to"].is_array())
    {
        throw std::runtime_error("Missing or invalid 'to' array");
    }

    for (const auto& item : json["to"])
    {
        trans.to.push_back(parseOutputArc(item));
    }

    if (json.contains("priority") && json["priority"].is_number())
    {
        trans.priority = json["priority"].get<int>();
    }

    return trans;
}

OutputArc ConfigParser::parseOutputArc(const nlohmann::json& json)
{
    OutputArc arc;

    if (json.is_string())
    {
        arc.to = json.get<std::string>();
    }
    else if (json.is_object())
    {
        if (json.contains("to"))
        {
            arc.to = json["to"].get<std::string>();
        }
        if (json.contains("token_filter"))
        {
            arc.tokenFilter = json["token_filter"].get<std::string>();
        }
    }

    return arc;
}

void ConfigParser::addError(const std::string& path, const std::string& message)
{
    result_.errors.push_back({path, message});
}

void ConfigParser::addWarning(const std::string& message)
{
    result_.warnings.push_back(message);
}

} // namespace bnet::config
