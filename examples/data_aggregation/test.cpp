// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file test.cpp
/// @brief Tests for the Data Aggregation example

#include <bnet/actors/DataStoreActor.hpp>
#include <bnet/actors/HttpActor.hpp>
#include <bnet/config/ConfigParser.hpp>
#include <bnet/runtime/RuntimeController.hpp>
#include <bnet/testing/TestHttpServer.hpp>

#include <cassert>
#include <iostream>

void testConfigIsValid()
{
    std::cout << "Testing config is valid..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseFile("examples/data_aggregation/config.json");

    assert(result.success && "Config should parse successfully");

    const auto& config = result.config;

    // Check actors
    assert(config.actors.size() == 2);
    assert(config.actors[0].id == "builtin::Http");
    assert(config.actors[1].id == "builtin::DataStore");

    // Check actions
    assert(config.actions.size() == 3);
    assert(config.actions[0].id == "builtin::http_get");
    assert(config.actions[1].id == "builtin::datastore_set");
    assert(config.actions[2].id == "builtin::datastore_get");

    // Check places
    assert(config.places.size() == 6);
    assert(config.places[0].id == "entry");
    assert(config.places[0].type == bnet::config::PlaceType::Entrypoint);
    assert(config.places[1].id == "fetch_weather_city1");
    assert(config.places[1].type == bnet::config::PlaceType::Action);

    // Check transitions
    assert(config.transitions.size() == 6);

    std::cout << "  PASSED" << std::endl;
}

void testWorkflowExecution()
{
    std::cout << "Testing workflow execution..." << std::endl;

    // Set up mock HTTP server
    bnet::testing::TestHttpServer httpServer;

    httpServer.get("/weather/sf", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{
            200, {}, R"({"city": "SF", "temp": 18})"
        };
    });

    httpServer.get("/weather/ny", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{
            200, {}, R"({"city": "NY", "temp": 25})"
        };
    });

    auto httpService = httpServer.createService();

    // Create actors
    bnet::actors::HttpActor httpActor(httpService);
    bnet::actors::DataStoreActor dataStoreActor;

    // Load config
    bnet::config::ConfigParser parser;
    auto configResult = parser.parseFile("examples/data_aggregation/config.json");
    assert(configResult.success);

    // Create controller
    bnet::runtime::RuntimeController controller;

    // Register actions
    controller.registerAction("builtin::http_get",
        [&httpActor](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return httpActor.get(token);
        });

    controller.registerAction("builtin::datastore_set",
        [&dataStoreActor](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return dataStoreActor.setValue(token);
        });

    controller.registerAction("builtin::datastore_get",
        [&dataStoreActor](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return dataStoreActor.getValue(token);
        });

    // Load config
    bool loaded = controller.loadConfig(configResult.config);
    assert(loaded && "Config should load successfully");

    // Inject token
    bnet::Token token;
    token.setData("url", "http://localhost/weather/sf");
    controller.injectToken("entry", std::move(token));

    // Run
    controller.start();

    int ticks = 0;
    bool isRunning = (controller.state() == bnet::runtime::RuntimeState::Running);
    while (isRunning && ticks < 50)
    {
        controller.tick();
        ++ticks;

        auto stats = controller.stats();
        if (stats.activeTokens == 0 && ticks > 1)
        {
            break;
        }

        isRunning = (controller.state() == bnet::runtime::RuntimeState::Running);
    }

    controller.stop();

    // Verify HTTP request was made
    assert(httpServer.requestCount() >= 1 && "Should have made at least one HTTP request");

    std::cout << "  PASSED (completed in " << ticks << " ticks)" << std::endl;
}

void testHttpActorIntegration()
{
    std::cout << "Testing HTTP actor integration..." << std::endl;

    // Set up mock server
    bnet::testing::TestHttpServer httpServer;
    httpServer.get("/api/data", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{
            200,
            {{"Content-Type", "application/json"}},
            R"({"status": "ok", "value": 42})"
        };
    });

    auto httpService = httpServer.createService();
    bnet::actors::HttpActor httpActor(httpService);

    // Create a token with URL
    bnet::Token token;
    token.setData("url", "http://localhost/api/data");

    // Execute GET
    auto result = httpActor.get(token);

    assert(result.status() == bnet::ActionResult::Status::Success);
    assert(httpServer.requestCount() == 1);
    assert(httpServer.lastRequest().method == "GET");
    assert(httpServer.lastRequest().path == "/api/data");

    // Check response was stored in token
    assert(token.hasData("response_body"));

    std::cout << "  PASSED" << std::endl;
}

void testDataStoreActorIntegration()
{
    std::cout << "Testing DataStore actor integration..." << std::endl;

    bnet::actors::DataStoreActor dataStore;

    // Test setValue
    bnet::Token token1;
    token1.setData("key", "my_key");
    token1.setData("value", nlohmann::json{{"foo", "bar"}});

    auto result1 = dataStore.setValue(token1);
    assert(result1.status() == bnet::ActionResult::Status::Success);
    assert(dataStore.has("my_key"));
    assert(dataStore.get("my_key")["foo"] == "bar");

    // Test getValue
    bnet::Token token2;
    token2.setData("key", "my_key");

    auto result2 = dataStore.getValue(token2);
    assert(result2.status() == bnet::ActionResult::Status::Success);
    assert(token2.hasData("result"));
    assert(token2.getData("result")["foo"] == "bar");

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== Data Aggregation Example Tests ===" << std::endl;

    testConfigIsValid();
    testHttpActorIntegration();
    testDataStoreActorIntegration();
    testWorkflowExecution();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
