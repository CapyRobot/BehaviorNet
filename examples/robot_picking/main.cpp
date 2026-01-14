// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file main.cpp
/// @brief Robot Picking Example - User-defined actors
///
/// This example demonstrates a BehaviorNet workflow with custom actors:
/// 1. A RobotActor that controls a simulated robot arm
/// 2. A ConveyorActor that controls a simulated conveyor belt
///
/// The workflow picks an item from the conveyor and places it at a destination.

#include "robot_actors.hpp"

#include <bnet/config/ConfigParser.hpp>
#include <bnet/runtime/RuntimeController.hpp>

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::cout << "=== BehaviorNet Robot Picking Example ===" << std::endl;
    std::cout << std::endl;

    // Step 1: Create the user-defined actors
    std::cout << "Creating actors..." << std::endl;
    robot_picking::RobotActor robot("robot_arm_1", 1.5);  // Robot ID and speed
    robot_picking::ConveyorActor conveyor("conveyor_main");
    std::cout << "  RobotActor '" << robot.robotId() << "': ready" << std::endl;
    std::cout << "  ConveyorActor '" << conveyor.conveyorId() << "': ready" << std::endl;

    // Step 2: Load the configuration
    std::cout << "Loading configuration..." << std::endl;

    std::string configPath = "examples/robot_picking/config.json";
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

    // Step 3: Create the runtime controller
    std::cout << "Creating runtime controller..." << std::endl;
    bnet::runtime::RuntimeController controller;

    // Register our custom actions using the helper function
    robot_picking::registerRobotPickingActions(controller, robot, conveyor);
    std::cout << "  6 custom actions registered" << std::endl;

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
    std::cout << "  Config loaded into controller" << std::endl;

    // Step 4: Create and inject the initial token
    std::cout << std::endl;
    std::cout << "Starting workflow execution..." << std::endl;

    bnet::Token token;

    // Set up the picking task parameters
    token.setData("task_id", "pick_001");

    // Pickup position (where items arrive on conveyor)
    token.setData("target_x", 100.0);  // Will be used for first move
    token.setData("target_y", 50.0);
    token.setData("target_z", 10.0);

    // Destination position (where to place items)
    token.setData("dropoff_x", 200.0);
    token.setData("dropoff_y", 50.0);
    token.setData("dropoff_z", 10.0);

    controller.injectToken("entry", std::move(token));
    std::cout << "  Token injected at 'entry' with task_id='pick_001'" << std::endl;

    // Step 5: Run the execution loop
    controller.start();
    std::cout << "  Controller started" << std::endl;
    std::cout << std::endl;
    std::cout << "Executing workflow:" << std::endl;

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
            std::cout << std::endl;
            std::cout << "Workflow completed after " << tickCount << " ticks" << std::endl;
            break;
        }

        isRunning = (controller.state() == bnet::runtime::RuntimeState::Running);
    }

    if (tickCount >= maxTicks)
    {
        std::cerr << "Warning: reached max ticks without completion" << std::endl;
    }

    controller.stop();

    // Step 6: Print results
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Robot final position: ("
              << robot.currentPosition().x << ", "
              << robot.currentPosition().y << ", "
              << robot.currentPosition().z << ")" << std::endl;
    std::cout << "Robot holding item: " << (robot.hasItem() ? "yes" : "no") << std::endl;
    std::cout << "Conveyor running: " << (conveyor.isRunning() ? "yes" : "no") << std::endl;

    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;
    return 0;
}
