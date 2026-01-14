// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file test.cpp
/// @brief Tests for the Robot Picking example

#include "robot_actors.hpp"

#include <bnet/config/ConfigParser.hpp>
#include <bnet/runtime/RuntimeController.hpp>

#include <cassert>
#include <iostream>

void testConfigIsValid()
{
    std::cout << "Testing config is valid..." << std::endl;

    bnet::config::ConfigParser parser;
    auto result = parser.parseFile("examples/robot_picking/config.json");

    assert(result.success && "Config should parse successfully");

    const auto& config = result.config;

    // Check actors
    assert(config.actors.size() == 2);
    assert(config.actors[0].id == "RobotActor");
    assert(config.actors[1].id == "ConveyorActor");

    // Check actions
    assert(config.actions.size() == 6);
    assert(config.actions[0].id == "robot::move_to_position");
    assert(config.actions[1].id == "robot::pick_item");
    assert(config.actions[2].id == "robot::place_item");
    assert(config.actions[3].id == "conveyor::start");
    assert(config.actions[4].id == "conveyor::stop");
    assert(config.actions[5].id == "conveyor::wait_for_item");

    // Check places
    assert(config.places.size() == 11);
    assert(config.places[0].id == "entry");
    assert(config.places[0].type == bnet::config::PlaceType::Entrypoint);

    // Check transitions
    assert(config.transitions.size() == 10);

    std::cout << "  PASSED" << std::endl;
}

void testRobotActorMoveToPosition()
{
    std::cout << "Testing RobotActor moveToPosition..." << std::endl;

    robot_picking::RobotActor robot("test_robot", 1.0);

    bnet::Token token;
    token.setData("target_x", 100.0);
    token.setData("target_y", 200.0);
    token.setData("target_z", 50.0);

    auto result = robot.moveToPosition(token);

    assert(result.status() == bnet::ActionResult::Status::Success);
    assert(robot.currentPosition().x == 100.0);
    assert(robot.currentPosition().y == 200.0);
    assert(robot.currentPosition().z == 50.0);
    assert(token.hasData("move_completed"));
    assert(token.getData("move_completed") == true);

    std::cout << "  PASSED" << std::endl;
}

void testRobotActorPickAndPlace()
{
    std::cout << "Testing RobotActor pick and place..." << std::endl;

    robot_picking::RobotActor robot("test_robot", 1.0);

    // Initially not holding item
    assert(!robot.hasItem());

    // Pick item
    bnet::Token pickToken;
    pickToken.setData("item_present", true);
    auto pickResult = robot.pickItem(pickToken);

    assert(pickResult.status() == bnet::ActionResult::Status::Success);
    assert(robot.hasItem());
    assert(pickToken.hasData("item_picked"));

    // Try to pick again - should fail
    bnet::Token pickToken2;
    pickToken2.setData("item_present", true);
    auto pickResult2 = robot.pickItem(pickToken2);
    assert(pickResult2.status() == bnet::ActionResult::Status::Failure);

    // Place item
    bnet::Token placeToken;
    auto placeResult = robot.placeItem(placeToken);

    assert(placeResult.status() == bnet::ActionResult::Status::Success);
    assert(!robot.hasItem());
    assert(placeToken.hasData("item_placed"));

    // Try to place again - should fail
    bnet::Token placeToken2;
    auto placeResult2 = robot.placeItem(placeToken2);
    assert(placeResult2.status() == bnet::ActionResult::Status::Failure);

    std::cout << "  PASSED" << std::endl;
}

void testConveyorActor()
{
    std::cout << "Testing ConveyorActor..." << std::endl;

    robot_picking::ConveyorActor conveyor("test_conveyor");

    // Initially not running
    assert(!conveyor.isRunning());

    // Start conveyor
    bnet::Token startToken;
    auto startResult = conveyor.start(startToken);
    assert(startResult.status() == bnet::ActionResult::Status::Success);
    assert(conveyor.isRunning());

    // Wait for item
    bnet::Token waitToken;
    auto waitResult = conveyor.waitForItem(waitToken);
    assert(waitResult.status() == bnet::ActionResult::Status::Success);
    assert(waitToken.hasData("item_present"));

    // Stop conveyor
    bnet::Token stopToken;
    auto stopResult = conveyor.stop(stopToken);
    assert(stopResult.status() == bnet::ActionResult::Status::Success);
    assert(!conveyor.isRunning());

    // Wait for item when stopped - should fail
    bnet::Token waitToken2;
    auto waitResult2 = conveyor.waitForItem(waitToken2);
    assert(waitResult2.status() == bnet::ActionResult::Status::Failure);

    std::cout << "  PASSED" << std::endl;
}

void testWorkflowExecution()
{
    std::cout << "Testing full workflow execution..." << std::endl;

    // Create actors
    robot_picking::RobotActor robot("robot1", 1.0);
    robot_picking::ConveyorActor conveyor("conv1");

    // Load config
    bnet::config::ConfigParser parser;
    auto configResult = parser.parseFile("examples/robot_picking/config.json");
    assert(configResult.success);

    // Create controller and register actions
    bnet::runtime::RuntimeController controller;
    robot_picking::registerRobotPickingActions(controller, robot, conveyor);

    // Load config
    bool loaded = controller.loadConfig(configResult.config);
    assert(loaded && "Config should load successfully");

    // Create token with task data
    bnet::Token token;
    token.setData("task_id", "test_001");
    token.setData("target_x", 100.0);
    token.setData("target_y", 50.0);
    token.setData("target_z", 10.0);

    controller.injectToken("entry", std::move(token));

    // Run workflow
    controller.start();

    int ticks = 0;
    bool isRunning = (controller.state() == bnet::runtime::RuntimeState::Running);
    while (isRunning && ticks < 100)
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

    // Verify workflow completed
    assert(ticks < 100 && "Workflow should complete within tick limit");

    // Verify final state
    // Conveyor should be stopped
    assert(!conveyor.isRunning());

    // Robot should have placed the item (not holding)
    assert(!robot.hasItem());

    std::cout << "  PASSED (completed in " << ticks << " ticks)" << std::endl;
}

void testActorRegistration()
{
    std::cout << "Testing action registration helper..." << std::endl;

    robot_picking::RobotActor robot("robot1", 1.0);
    robot_picking::ConveyorActor conveyor("conv1");
    bnet::runtime::RuntimeController controller;

    // This should not throw
    robot_picking::registerRobotPickingActions(controller, robot, conveyor);

    // Verify we can load config that uses these actions
    bnet::config::ConfigParser parser;
    auto configResult = parser.parseFile("examples/robot_picking/config.json");
    assert(configResult.success);

    bool loaded = controller.loadConfig(configResult.config);
    assert(loaded);

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== Robot Picking Example Tests ===" << std::endl;

    testConfigIsValid();
    testRobotActorMoveToPosition();
    testRobotActorPickAndPlace();
    testConveyorActor();
    testActorRegistration();
    testWorkflowExecution();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
