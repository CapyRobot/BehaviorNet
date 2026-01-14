// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file main.cpp
/// @brief Data Aggregation Example - Using only built-in actors
///
/// This example demonstrates a BehaviorNet workflow that:
/// 1. Fetches weather data from multiple cities via HTTP
/// 2. Aggregates results into a DataStore
/// 3. Exits with the combined data

#include <bnet/actors/DataStoreActor.hpp>
#include <bnet/actors/HttpActor.hpp>
#include <bnet/config/ConfigParser.hpp>
#include <bnet/runtime/RuntimeController.hpp>
#include <bnet/testing/TestHttpServer.hpp>

#include <iostream>
#include <string>

namespace {

/// @brief Set up mock weather API endpoints on the test server.
void setupWeatherApi(bnet::testing::TestHttpServer& server)
{
    // Mock weather API for city 1 (San Francisco)
    server.get("/weather/san-francisco", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{
            200,
            {{"Content-Type", "application/json"}},
            R"({"city": "San Francisco", "temp_c": 18, "conditions": "Foggy"})"
        };
    });

    // Mock weather API for city 2 (New York)
    server.get("/weather/new-york", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{
            200,
            {{"Content-Type", "application/json"}},
            R"({"city": "New York", "temp_c": 25, "conditions": "Sunny"})"
        };
    });

    // Default handler for unknown routes
    server.setDefaultHandler([](const bnet::testing::TestHttpServer::Request& req)
    {
        std::cerr << "Unknown route: " << req.method << " " << req.path << std::endl;
        return bnet::testing::TestHttpServer::Response{404, {}, "Not Found"};
    });
}

/// @brief Register built-in action invokers with the runtime.
void registerBuiltinActions(bnet::runtime::RuntimeController& controller,
                            bnet::actors::HttpActor& httpActor,
                            bnet::actors::DataStoreActor& dataStoreActor)
{
    // Register HTTP GET action
    controller.registerAction("builtin::http_get",
        [&httpActor](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return httpActor.get(token);
        });

    // Register DataStore SET action
    controller.registerAction("builtin::datastore_set",
        [&dataStoreActor](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return dataStoreActor.setValue(token);
        });

    // Register DataStore GET action
    controller.registerAction("builtin::datastore_get",
        [&dataStoreActor](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return dataStoreActor.getValue(token);
        });
}

} // namespace

int main(int argc, char* argv[])
{
    std::cout << "=== BehaviorNet Data Aggregation Example ===" << std::endl;
    std::cout << std::endl;

    // Step 1: Set up the test HTTP server
    std::cout << "Setting up mock weather API..." << std::endl;
    bnet::testing::TestHttpServer httpServer;
    setupWeatherApi(httpServer);
    auto httpService = httpServer.createService();
    std::cout << "  Mock API ready" << std::endl;

    // Step 2: Create the built-in actors
    std::cout << "Creating actors..." << std::endl;
    bnet::actors::HttpActor httpActor(httpService);
    bnet::actors::DataStoreActor dataStoreActor;
    std::cout << "  HttpActor: ready" << std::endl;
    std::cout << "  DataStoreActor: ready" << std::endl;

    // Step 3: Load the configuration
    std::cout << "Loading configuration..." << std::endl;

    // Determine config path - use command line arg or default
    std::string configPath = "examples/data_aggregation/config.json";
    if (argc > 1)
    {
        configPath = argv[1];
    }

    bnet::config::ConfigParser parser;
    auto configResult = parser.parseFile(configPath);
    if (!configResult.success)
    {
        std::cerr << "Failed to load config: " << configPath << std::endl;
        for (const auto& err : configResult.errors)
        {
            std::cerr << "  " << err.path << ": " << err.message << std::endl;
        }
        return 1;
    }
    std::cout << "  Loaded: " << configPath << std::endl;

    // Step 4: Create the runtime controller
    std::cout << "Creating runtime controller..." << std::endl;
    bnet::runtime::RuntimeController controller;

    // Register the built-in actions
    registerBuiltinActions(controller, httpActor, dataStoreActor);
    std::cout << "  Actions registered" << std::endl;

    // Load the config into the controller
    if (!controller.loadConfig(configResult.config))
    {
        std::cerr << "Failed to load config into controller" << std::endl;
        for (const auto& err : controller.errors())
        {
            std::cerr << "  " << err << std::endl;
        }
        return 1;
    }
    std::cout << "  Config loaded" << std::endl;

    // Step 5: Create and inject the initial token
    std::cout << std::endl;
    std::cout << "Starting workflow execution..." << std::endl;

    bnet::Token token;
    // Set up the token with the cities to fetch
    token.setData("cities", nlohmann::json::array({"san-francisco", "new-york"}));
    token.setData("current_city_index", 0);

    // For the first HTTP call
    token.setData("url", "http://localhost/weather/san-francisco");

    controller.injectToken("entry", std::move(token));
    std::cout << "  Token injected at 'entry'" << std::endl;

    // Step 6: Run the execution loop
    controller.start();
    std::cout << "  Controller started" << std::endl;

    // Tick until completion (in a real app, this would be event-driven)
    int maxTicks = 100;
    int tickCount = 0;
    bool isRunning = (controller.state() == bnet::runtime::RuntimeState::Running);
    while (isRunning && tickCount < maxTicks)
    {
        controller.tick();
        ++tickCount;

        // Check for completed tokens by looking at stats
        auto stats = controller.stats();
        if (stats.activeTokens == 0 && tickCount > 1)
        {
            std::cout << "  Workflow completed after " << tickCount << " ticks" << std::endl;
            break;
        }

        isRunning = (controller.state() == bnet::runtime::RuntimeState::Running);
    }

    if (tickCount >= maxTicks)
    {
        std::cerr << "  Warning: reached max ticks without completion" << std::endl;
    }

    controller.stop();

    // Step 7: Print results
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "HTTP requests made: " << httpServer.requestCount() << std::endl;

    for (const auto& req : httpServer.requests())
    {
        std::cout << "  " << req.method << " " << req.path << std::endl;
    }

    // Print aggregated data from the DataStore
    std::cout << std::endl;
    std::cout << "DataStore contents:" << std::endl;
    if (dataStoreActor.has("weather_results"))
    {
        auto results = dataStoreActor.get("weather_results");
        std::cout << "  weather_results: " << results.dump(2) << std::endl;
    }
    else
    {
        std::cout << "  (no weather_results stored)" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;
    return 0;
}
