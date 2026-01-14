// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file warehouse_actors.hpp
/// @brief Example actors for the autonomous warehouse use case.
///
/// This file demonstrates how to define actors and actions for BehaviorNet.
/// See design/usecases/autonomous_warehouse.md for the full use case.

#pragma once

#include <bnet/bnet.hpp>
#include <string>

namespace warehouse {

// =============================================================================
// AMR (Autonomous Mobile Robot) Actor
// =============================================================================

/// Actor representing an Autonomous Mobile Robot.
///
/// AMRs transport bins between locations in the warehouse.
class AMRActor : public bnet::ActorBase {
public:
    explicit AMRActor(const bnet::ActorParams& params)
        : id_(params.get("id"))
        , address_(params.get("Addr"))
        , metadata_(params.getOr("metadata", ""))
    {
        // In a real implementation, you would connect to the robot here
        // assertValidConnection();
    }

    /// Transport bins to a destination specified in the token.
    ///
    /// The token should contain a station actor to get the destination from.
    bnet::ActionResult transportBins(const bnet::Token& token) {
        // In a real implementation:
        // auto& station = token.getActor<StationActor>();
        // auto destination = station.getLocation();
        // return sendTransportCommand(destination);

        // Placeholder implementation
        return bnet::ActionResult::success();
    }

    /// Check if the robot's battery is sufficiently charged.
    ///
    /// Returns success if battery > 80%, failure otherwise.
    bnet::ActionResult isCharged() {
        int batteryLevel = getBatteryLevel();
        if (batteryLevel > 80) {
            return bnet::ActionResult::success();
        }
        return bnet::ActionResult::failure();
    }

    /// Navigate to a charging station and charge the battery.
    bnet::ActionResult charge() {
        // In a real implementation:
        // return sendChargeCommand();

        return bnet::ActionResult::inProgress();  // Charging takes time
    }

    const std::string& getId() const { return id_; }
    const std::string& getAddress() const { return address_; }

private:
    int getBatteryLevel() const {
        // Placeholder - would query actual robot
        return 85;
    }

    std::string id_;
    std::string address_;
    std::string metadata_;
};

// =============================================================================
// Bin Picking Station Actor
// =============================================================================

/// Actor representing a bin picking station.
///
/// Bin picking stations extract items from bins for order fulfillment.
class BinPickingStationActor : public bnet::ActorBase {
public:
    explicit BinPickingStationActor(const bnet::ActorParams& params)
        : id_(params.get("id"))
        , address_(params.get("Addr"))
    {}

    /// Execute a picking operation for the order in the token.
    bnet::ActionResult executeOrder(const bnet::Token& /* token */) {
        // In a real implementation, you would get order info from an OrderActor:
        // auto& order = token.getActor<OrderActor>();
        // return sendPickCommand(order.getId());

        return bnet::ActionResult::success();
    }

    std::string getLocation() const {
        return "station_" + id_;
    }

    const std::string& getId() const { return id_; }

private:
    std::string id_;
    std::string address_;
};

// =============================================================================
// Packing Station Actor
// =============================================================================

/// Actor representing a packing station.
///
/// Packing stations pack items for shipment after picking.
class PackingStationActor : public bnet::ActorBase {
public:
    explicit PackingStationActor(const bnet::ActorParams& params)
        : id_(params.get("id"))
        , address_(params.get("Addr"))
    {}

    /// Pack the items from the current order.
    bnet::ActionResult pack() {
        // In a real implementation:
        // return sendPackCommand();

        return bnet::ActionResult::success();
    }

    /// Notify that packing is complete.
    bnet::ActionResult notifyDone() {
        // In a real implementation:
        // return sendNotification();

        return bnet::ActionResult::success();
    }

    std::string getLocation() const {
        return "packing_" + id_;
    }

private:
    std::string id_;
    std::string address_;
};

} // namespace warehouse

// =============================================================================
// Actor and Action Registration
// =============================================================================

// Register actor types
BNET_REGISTER_ACTOR(warehouse::AMRActor, "AMR");
BNET_REGISTER_ACTOR(warehouse::BinPickingStationActor, "BinPickingStation");
BNET_REGISTER_ACTOR(warehouse::PackingStationActor, "PackingStation");

// Register AMR actions
BNET_REGISTER_ACTOR_ACTION(warehouse::AMRActor, isCharged, "is_charged");
BNET_REGISTER_ACTOR_ACTION(warehouse::AMRActor, charge, "charge");
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN(warehouse::AMRActor, transportBins, "transport_bins");

// Register BinPickingStation actions
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN(warehouse::BinPickingStationActor, executeOrder, "execute_order");

// Register PackingStation actions
BNET_REGISTER_ACTOR_ACTION(warehouse::PackingStationActor, pack, "pack");
BNET_REGISTER_ACTOR_ACTION(warehouse::PackingStationActor, notifyDone, "notify_done");
