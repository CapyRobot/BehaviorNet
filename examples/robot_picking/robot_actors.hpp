// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file robot_actors.hpp
/// @brief User-defined actors for the Robot Picking example
///
/// This file demonstrates how to create custom actors for BehaviorNet.
/// Users define their own actors by:
/// 1. Extending bnet::ActorBase
/// 2. Implementing action methods that take a Token& and return ActionResult
/// 3. Registering action invokers with the RuntimeController

#include <bnet/ActionResult.hpp>
#include <bnet/Actor.hpp>
#include <bnet/Token.hpp>
#include <bnet/runtime/RuntimeController.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace robot_picking {

/// @brief Represents a 3D position in the workspace.
struct Position
{
    double x{0.0};
    double y{0.0};
    double z{0.0};

    bool operator==(const Position& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

/// @brief Simulated robot arm actor.
///
/// This actor simulates a robot arm that can move to positions and pick/place items.
/// In a real application, this would interface with the actual robot hardware.
class RobotActor : public bnet::ActorBase
{
public:
    /// @brief Create a robot actor with the given ID.
    explicit RobotActor(const std::string& robotId, double speed = 1.0)
        : robotId_(robotId)
        , speed_(speed)
        , currentPosition_{0.0, 0.0, 0.0}
        , hasItem_(false)
    {
    }

    /// @brief Move the robot to a target position.
    ///
    /// Reads target position from token data:
    /// - "target_x", "target_y", "target_z" - target coordinates
    ///
    /// Writes to token data:
    /// - "move_completed" - true when movement is done
    bnet::ActionResult moveToPosition(bnet::Token& token)
    {
        // Read target position from token
        double targetX = token.getDataOr("target_x", 0.0).get<double>();
        double targetY = token.getDataOr("target_y", 0.0).get<double>();
        double targetZ = token.getDataOr("target_z", 0.0).get<double>();

        Position target{targetX, targetY, targetZ};

        std::cout << "  [" << robotId_ << "] Moving to ("
                  << targetX << ", " << targetY << ", " << targetZ << ")" << std::endl;

        // Simulate movement (instant for this example)
        currentPosition_ = target;

        token.setData("move_completed", true);
        token.setData("robot_position", nlohmann::json{
            {"x", currentPosition_.x},
            {"y", currentPosition_.y},
            {"z", currentPosition_.z}
        });

        return bnet::ActionResult::success();
    }

    /// @brief Pick up an item at the current position.
    ///
    /// Reads from token data:
    /// - "item_present" - whether an item is available to pick (optional)
    ///
    /// Writes to token data:
    /// - "item_picked" - true if item was successfully picked
    bnet::ActionResult pickItem(bnet::Token& token)
    {
        if (hasItem_)
        {
            std::cout << "  [" << robotId_ << "] Already holding an item!" << std::endl;
            return bnet::ActionResult::failure("already_holding_item");
        }

        // Check if item is present (simulated)
        bool itemPresent = token.getDataOr("item_present", true).get<bool>();

        if (!itemPresent)
        {
            std::cout << "  [" << robotId_ << "] No item to pick at current position" << std::endl;
            return bnet::ActionResult::failure("no_item_present");
        }

        std::cout << "  [" << robotId_ << "] Picking item at ("
                  << currentPosition_.x << ", " << currentPosition_.y << ")" << std::endl;

        hasItem_ = true;
        token.setData("item_picked", true);
        token.setData("pick_position", nlohmann::json{
            {"x", currentPosition_.x},
            {"y", currentPosition_.y}
        });

        return bnet::ActionResult::success();
    }

    /// @brief Place the held item at the current position.
    ///
    /// Writes to token data:
    /// - "item_placed" - true if item was placed
    /// - "place_position" - coordinates where item was placed
    bnet::ActionResult placeItem(bnet::Token& token)
    {
        if (!hasItem_)
        {
            std::cout << "  [" << robotId_ << "] No item to place!" << std::endl;
            return bnet::ActionResult::failure("no_item_held");
        }

        std::cout << "  [" << robotId_ << "] Placing item at ("
                  << currentPosition_.x << ", " << currentPosition_.y << ")" << std::endl;

        hasItem_ = false;
        token.setData("item_placed", true);
        token.setData("place_position", nlohmann::json{
            {"x", currentPosition_.x},
            {"y", currentPosition_.y}
        });

        return bnet::ActionResult::success();
    }

    // Accessors for testing
    [[nodiscard]] const Position& currentPosition() const { return currentPosition_; }
    [[nodiscard]] bool hasItem() const { return hasItem_; }
    [[nodiscard]] const std::string& robotId() const { return robotId_; }

private:
    std::string robotId_;
    double speed_;
    Position currentPosition_;
    bool hasItem_;
};

/// @brief Simulated conveyor belt actor.
///
/// This actor simulates a conveyor belt that transports items.
/// In a real application, this would interface with the conveyor hardware/PLC.
class ConveyorActor : public bnet::ActorBase
{
public:
    /// @brief Create a conveyor actor with the given ID.
    explicit ConveyorActor(const std::string& conveyorId)
        : conveyorId_(conveyorId)
        , running_(false)
        , itemAtPickup_(false)
    {
    }

    /// @brief Start the conveyor belt.
    bnet::ActionResult start(bnet::Token& token)
    {
        if (running_)
        {
            std::cout << "  [" << conveyorId_ << "] Already running" << std::endl;
            return bnet::ActionResult::success();
        }

        std::cout << "  [" << conveyorId_ << "] Starting conveyor" << std::endl;
        running_ = true;
        token.setData("conveyor_running", true);

        return bnet::ActionResult::success();
    }

    /// @brief Stop the conveyor belt.
    bnet::ActionResult stop(bnet::Token& token)
    {
        if (!running_)
        {
            std::cout << "  [" << conveyorId_ << "] Already stopped" << std::endl;
            return bnet::ActionResult::success();
        }

        std::cout << "  [" << conveyorId_ << "] Stopping conveyor" << std::endl;
        running_ = false;
        token.setData("conveyor_running", false);

        return bnet::ActionResult::success();
    }

    /// @brief Wait for an item to arrive at the pickup position.
    ///
    /// In this simulation, we immediately have an item available.
    /// In a real system, this might return IN_PROGRESS until a sensor detects an item.
    bnet::ActionResult waitForItem(bnet::Token& token)
    {
        if (!running_)
        {
            std::cout << "  [" << conveyorId_ << "] Cannot wait for item - conveyor not running"
                      << std::endl;
            return bnet::ActionResult::failure("conveyor_not_running");
        }

        // Simulate item arrival (immediate for this example)
        std::cout << "  [" << conveyorId_ << "] Item arrived at pickup position" << std::endl;
        itemAtPickup_ = true;

        token.setData("item_present", true);
        token.setData("item_ready_time", std::chrono::system_clock::now().time_since_epoch().count());

        return bnet::ActionResult::success();
    }

    /// @brief Simulate an item being picked (called by robot).
    void itemPicked()
    {
        itemAtPickup_ = false;
    }

    // Accessors for testing
    [[nodiscard]] bool isRunning() const { return running_; }
    [[nodiscard]] bool hasItemAtPickup() const { return itemAtPickup_; }
    [[nodiscard]] const std::string& conveyorId() const { return conveyorId_; }

private:
    std::string conveyorId_;
    bool running_;
    bool itemAtPickup_;
};

/// @brief Helper to register all robot picking actions with a RuntimeController.
///
/// Usage:
/// ```cpp
/// RobotActor robot("robot1");
/// ConveyorActor conveyor("conv1");
/// registerRobotPickingActions(controller, robot, conveyor);
/// ```
inline void registerRobotPickingActions(
    bnet::runtime::RuntimeController& controller,
    RobotActor& robot,
    ConveyorActor& conveyor)
{
    // Robot actions
    controller.registerAction("robot::move_to_position",
        [&robot](bnet::ActorBase*, bnet::Token& token) {
            return robot.moveToPosition(token);
        });

    controller.registerAction("robot::pick_item",
        [&robot](bnet::ActorBase*, bnet::Token& token) {
            return robot.pickItem(token);
        });

    controller.registerAction("robot::place_item",
        [&robot](bnet::ActorBase*, bnet::Token& token) {
            return robot.placeItem(token);
        });

    // Conveyor actions
    controller.registerAction("conveyor::start",
        [&conveyor](bnet::ActorBase*, bnet::Token& token) {
            return conveyor.start(token);
        });

    controller.registerAction("conveyor::stop",
        [&conveyor](bnet::ActorBase*, bnet::Token& token) {
            return conveyor.stop(token);
        });

    controller.registerAction("conveyor::wait_for_item",
        [&conveyor](bnet::ActorBase*, bnet::Token& token) {
            return conveyor.waitForItem(token);
        });
}

} // namespace robot_picking
