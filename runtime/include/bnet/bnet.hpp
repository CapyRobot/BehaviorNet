// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file bnet.hpp
/// @brief Main include header for BehaviorNet user-facing API.
///
/// This header includes all public API headers needed to define actors
/// and actions for the BehaviorNet runtime.
///
/// Example usage:
/// @code
/// #include <bnet/bnet.hpp>
///
/// class MyRobot : public bnet::ActorBase {
/// public:
///     MyRobot(const bnet::ActorParams& params)
///         : id_(params.get("id"))
///         , address_(params.get("address"))
///     {}
///
///     bnet::ActionResult moveToLocation(const bnet::Token& token) {
///         auto& dest = token.getActor<DestinationActor>();
///         // ... implementation ...
///         return bnet::ActionResult::success();
///     }
///
///     bnet::ActionResult checkBattery() {
///         if (getBatteryLevel() > 20)
///             return bnet::ActionResult::success();
///         return bnet::ActionResult::failure();
///     }
///
///     bnet::ActionResult connectToServer() {
///         if (!canConnect())
///             return bnet::ActionResult::error<bnet::error::ConnectionError>(
///                 "Failed to connect", serverAddress_);
///         return bnet::ActionResult::success();
///     }
/// };
///
/// BNET_REGISTER_ACTOR(MyRobot, "Robot");
/// BNET_REGISTER_ACTOR_ACTION(MyRobot, checkBattery, "check_battery");
/// BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN(MyRobot, moveToLocation, "move_to_location");
/// @endcode

#pragma once

#include "ActionResult.hpp"
#include "Actor.hpp"
#include "Error.hpp"
#include "Token.hpp"
#include "Registry.hpp"
