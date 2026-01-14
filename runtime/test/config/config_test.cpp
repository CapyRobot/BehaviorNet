// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/config/ConfigParser.hpp>

#include <cassert>
#include <iostream>

void testParseMinimalConfig()
{
    std::cout << "Testing parse minimal config..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString(R"({
        "places": [
            {"id": "start", "type": "entrypoint"},
            {"id": "end", "type": "exit_logger"}
        ],
        "transitions": [
            {"from": ["start"], "to": ["end"]}
        ]
    })");

    assert(result.success);
    assert(result.errors.empty());
    assert(result.config.places.size() == 2);
    assert(result.config.transitions.size() == 1);

    assert(result.config.places[0].id == "start");
    assert(result.config.places[0].type == bnet::config::PlaceType::Entrypoint);

    assert(result.config.places[1].id == "end");
    assert(result.config.places[1].type == bnet::config::PlaceType::ExitLogger);

    assert(result.config.transitions[0].from.size() == 1);
    assert(result.config.transitions[0].from[0] == "start");
    assert(result.config.transitions[0].to.size() == 1);
    assert(result.config.transitions[0].to[0].to == "end");

    std::cout << "  PASSED" << std::endl;
}

void testParseActors()
{
    std::cout << "Testing parse actors..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString(R"({
        "actors": [
            {
                "id": "user::Robot",
                "required_init_params": {
                    "id": {"type": "str"},
                    "addr": {"type": "str"}
                },
                "optional_init_params": {
                    "name": {"type": "str"}
                }
            }
        ],
        "places": [],
        "transitions": []
    })");

    assert(result.success);
    assert(result.config.actors.size() == 1);

    const auto& actor = result.config.actors[0];
    assert(actor.id == "user::Robot");
    assert(actor.requiredInitParams.size() == 2);
    assert(actor.requiredInitParams.at("id").type == "str");
    assert(actor.optionalInitParams.size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testParseActions()
{
    std::cout << "Testing parse actions..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString(R"({
        "actions": [
            {
                "id": "user::move_to",
                "required_actors": ["user::Robot", "user::Location"]
            }
        ],
        "places": [],
        "transitions": []
    })");

    assert(result.success);
    assert(result.config.actions.size() == 1);

    const auto& action = result.config.actions[0];
    assert(action.id == "user::move_to");
    assert(action.requiredActors.size() == 2);
    assert(action.requiredActors[0] == "user::Robot");

    std::cout << "  PASSED" << std::endl;
}

void testParsePlaceTypes()
{
    std::cout << "Testing parse place types..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString(R"({
        "places": [
            {"id": "p1", "type": "plain"},
            {"id": "p2", "type": "entrypoint", "params": {"new_actors": ["Robot"]}},
            {"id": "p3", "type": "resource_pool", "params": {"resource_id": "Robot", "initial_availability": 5}},
            {"id": "p4", "type": "wait_with_timeout", "params": {"timeout_min": 10}},
            {"id": "p5", "type": "action", "params": {"action_id": "move", "retries": 2}},
            {"id": "p6", "type": "exit_logger"}
        ],
        "transitions": []
    })");

    assert(result.success);
    assert(result.config.places.size() == 6);

    assert(result.config.places[0].type == bnet::config::PlaceType::Plain);
    assert(result.config.places[1].type == bnet::config::PlaceType::Entrypoint);
    assert(result.config.places[2].type == bnet::config::PlaceType::ResourcePool);
    assert(result.config.places[3].type == bnet::config::PlaceType::WaitWithTimeout);
    assert(result.config.places[4].type == bnet::config::PlaceType::Action);
    assert(result.config.places[5].type == bnet::config::PlaceType::ExitLogger);

    // Check resource pool params
    const auto& poolParams = std::get<bnet::config::ResourcePoolParams>(result.config.places[2].params);
    assert(poolParams.resourceId == "Robot");
    assert(poolParams.initialAvailability == 5);

    // Check action params
    const auto& actionParams = std::get<bnet::config::ActionPlaceParams>(result.config.places[4].params);
    assert(actionParams.actionId == "move");
    assert(actionParams.retries == 2);

    std::cout << "  PASSED" << std::endl;
}

void testParseTransitionsWithFilters()
{
    std::cout << "Testing parse transitions with filters..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString(R"({
        "places": [
            {"id": "action::success"},
            {"id": "pool"},
            {"id": "next"}
        ],
        "transitions": [
            {
                "from": ["action::success"],
                "to": [
                    {"to": "next", "token_filter": "Robot"},
                    {"to": "pool", "token_filter": "Resource"}
                ]
            }
        ]
    })");

    assert(result.success);
    assert(result.config.transitions.size() == 1);

    const auto& trans = result.config.transitions[0];
    assert(trans.from.size() == 1);
    assert(trans.from[0] == "action::success");

    assert(trans.to.size() == 2);
    assert(trans.to[0].to == "next");
    assert(trans.to[0].tokenFilter.has_value());
    assert(*trans.to[0].tokenFilter == "Robot");

    assert(trans.to[1].to == "pool");
    assert(*trans.to[1].tokenFilter == "Resource");

    std::cout << "  PASSED" << std::endl;
}

void testParseInvalidJson()
{
    std::cout << "Testing parse invalid JSON..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString("not valid json");

    assert(!result.success);
    assert(!result.errors.empty());

    std::cout << "  PASSED" << std::endl;
}

void testParseMissingRequired()
{
    std::cout << "Testing parse missing required fields..." << std::endl;

    bnet::config::ConfigParser parser;

    // Missing places
    auto result1 = parser.parseString(R"({"transitions": []})");
    assert(!result1.success);

    // Missing transitions
    auto result2 = parser.parseString(R"({"places": []})");
    assert(!result2.success);

    std::cout << "  PASSED" << std::endl;
}

void testParseActionPlaceParams()
{
    std::cout << "Testing parse action place params..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseString(R"({
        "places": [{
            "id": "do_action",
            "type": "action",
            "params": {
                "action_id": "user::transport",
                "retries": 3,
                "timeout_per_try_min": 5,
                "failure_as_error": true,
                "error_to_global_handler": false
            }
        }],
        "transitions": []
    })");

    assert(result.success);
    const auto& params = std::get<bnet::config::ActionPlaceParams>(result.config.places[0].params);

    assert(params.actionId == "user::transport");
    assert(params.retries == 3);
    assert(params.timeoutPerTry == std::chrono::minutes(5));
    assert(params.failureAsError == true);
    assert(params.errorToGlobalHandler == false);

    std::cout << "  PASSED" << std::endl;
}

void testParseTimeoutParams()
{
    std::cout << "Testing parse timeout params..." << std::endl;

    bnet::config::ConfigParser parser;

    // Minutes
    auto result1 = parser.parseString(R"({
        "places": [{
            "id": "wait",
            "type": "wait_with_timeout",
            "params": {"timeout_min": 10, "on_timeout": "error::timeout"}
        }],
        "transitions": []
    })");

    assert(result1.success);
    const auto& params1 = std::get<bnet::config::WaitWithTimeoutParams>(result1.config.places[0].params);
    assert(params1.timeout == std::chrono::minutes(10));
    assert(params1.onTimeout == "error::timeout");

    // Seconds
    auto result2 = parser.parseString(R"({
        "places": [{
            "id": "wait",
            "type": "wait_with_timeout",
            "params": {"timeout_s": 30}
        }],
        "transitions": []
    })");

    assert(result2.success);
    const auto& params2 = std::get<bnet::config::WaitWithTimeoutParams>(result2.config.places[0].params);
    assert(params2.timeout == std::chrono::seconds(30));

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Config Tests ===" << std::endl;

    testParseMinimalConfig();
    testParseActors();
    testParseActions();
    testParsePlaceTypes();
    testParseTransitionsWithFilters();
    testParseInvalidJson();
    testParseMissingRequired();
    testParseActionPlaceParams();
    testParseTimeoutParams();

    std::cout << "\n=== All config tests passed ===" << std::endl;
    return 0;
}
