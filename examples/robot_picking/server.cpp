// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file server.cpp
/// @brief Robot picking workflow with WebSocket server for GUI integration.
///
/// This server runs the robot picking workflow and exposes it via WebSocket
/// for the BehaviorNet GUI to connect, monitor, and control.

#include "robot_actors.hpp"

#include <bnet/bnet.hpp>
#include <bnet/runtime/RuntimeController.hpp>
#include <bnet/server/WebSocketServer.hpp>

#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace {

std::atomic<bool> g_running{true};

void signalHandler(int signum)
{
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
}

std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/// @brief Helper to register all robot picking actions with the runtime.
void registerRobotActions(bnet::runtime::RuntimeController& runtime)
{
    // Create shared actors (would normally be created per-token)
    static auto robot = std::make_unique<robot_picking::RobotActor>("robot1");
    static auto conveyor = std::make_unique<robot_picking::ConveyorActor>("conv1");

    // Register action invokers
    runtime.registerAction("conveyor.start",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return conveyor->start(token);
        });

    runtime.registerAction("conveyor.stop",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return conveyor->stop(token);
        });

    runtime.registerAction("conveyor.waitForItem",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return conveyor->waitForItem(token);
        });

    runtime.registerAction("robot.moveToPickup",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            token.setData("target_x", 100);
            token.setData("target_y", 50);
            token.setData("target_z", 10);
            return robot->moveToPosition(token);
        });

    runtime.registerAction("robot.moveToPlace",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            token.setData("target_x", 100);
            token.setData("target_y", 50);
            token.setData("target_z", 10);
            return robot->moveToPosition(token);
        });

    runtime.registerAction("robot.pick",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return robot->pickItem(token);
        });

    runtime.registerAction("robot.place",
        [](bnet::ActorBase*, bnet::Token& token) -> bnet::ActionResult
        {
            return robot->placeItem(token);
        });
}

} // namespace

int main(int argc, char* argv[])
{
    // Default configuration
    std::string configPath = "examples/robot_picking/config.json";
    uint16_t port = 8080;

    // Parse arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc)
        {
            configPath = argv[++i];
        }
        else if ((arg == "-p" || arg == "--port") && i + 1 < argc)
        {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -c, --config PATH  Configuration file (default: examples/robot_picking/config.json)\n"
                      << "  -p, --port PORT    WebSocket server port (default: 8080)\n"
                      << "  -h, --help         Show this help message\n";
            return 0;
        }
    }

    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Read configuration
        std::string configJson = readFile(configPath);

        // Create runtime controller
        bnet::runtime::RuntimeController runtime;

        // Set up logging
        runtime.setLogCallback([](const std::string& message)
        {
            std::cout << "[Runtime] " << message << std::endl;
        });

        // Register actions
        registerRobotActions(runtime);

        // Load configuration
        if (!runtime.loadConfigString(configJson))
        {
            std::cerr << "Failed to load configuration:" << std::endl;
            for (const auto& error : runtime.errors())
            {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }

        std::cout << "Configuration loaded successfully" << std::endl;

        // Create and start WebSocket server
        bnet::server::WebSocketServer server(runtime, port);
        server.start();

        std::cout << "WebSocket server started on port " << port << std::endl;
        std::cout << "Connect the BehaviorNet GUI to ws://localhost:" << port << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        // Start the runtime
        runtime.start();

        // Main loop - just wait for shutdown signal
        while (g_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Print stats periodically
            static int counter = 0;
            if (++counter >= 50) // Every 5 seconds
            {
                counter = 0;
                auto stats = runtime.stats();
                std::cout << "[Stats] Epoch: " << stats.epoch
                          << ", Transitions fired: " << stats.transitionsFired
                          << ", Active tokens: " << stats.activeTokens
                          << ", Clients connected: " << server.clientCount()
                          << std::endl;
            }
        }

        // Clean shutdown
        std::cout << "Stopping server..." << std::endl;
        server.stop();
        runtime.stop();

        std::cout << "Server stopped" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
