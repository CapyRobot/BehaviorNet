// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

/// @file api_test.cpp
/// @brief Tests for the BehaviorNet user-facing API.

#include <bnet/bnet.hpp>
#include "runtime/examples/warehouse_actors.hpp"

#include <cassert>
#include <iostream>

void testActionResult() {
    std::cout << "Testing ActionResult..." << std::endl;

    // Success
    auto success = bnet::ActionResult::success();
    assert(success.isSuccess());
    assert(!success.isFailure());
    assert(!success.isInProgress());
    assert(!success.isError());
    assert(success.isTerminal());

    // Failure
    auto failure = bnet::ActionResult::failure();
    assert(!failure.isSuccess());
    assert(failure.isFailure());
    assert(failure.isTerminal());

    // In Progress
    auto inProgress = bnet::ActionResult::inProgress();
    assert(inProgress.isInProgress());
    assert(!inProgress.isTerminal());

    // Error with exception type
    auto error = bnet::ActionResult::error<bnet::error::TimeoutError>(
        "Connection timed out", 30);
    assert(error.isError());
    assert(error.isTerminal());
    assert(error.isErrorType<bnet::error::TimeoutError>());
    assert(error.isErrorType<bnet::error::NetworkError>());  // Parent type
    assert(error.isErrorType<bnet::error::RuntimeError>());  // Grandparent
    assert(!error.isErrorType<bnet::error::ConnectionError>());  // Sibling
    assert(error.errorMessage() == "Connection timed out");
    assert(error.errorTypeName() == "bnet::error::TimeoutError");

    std::cout << "  PASSED" << std::endl;
}

void testErrorHierarchy() {
    std::cout << "Testing Error hierarchy..." << std::endl;

    // Test that inheritance works for catching
    bool caughtAsNetwork = false;
    bool caughtAsRuntime = false;
    bool caughtAsError = false;

    try {
        throw bnet::error::TimeoutError("test", 10);
    } catch (const bnet::error::NetworkError&) {
        caughtAsNetwork = true;
    }
    assert(caughtAsNetwork);

    try {
        throw bnet::error::ConnectionError("test", "localhost");
    } catch (const bnet::error::RuntimeError&) {
        caughtAsRuntime = true;
    }
    assert(caughtAsRuntime);

    try {
        throw bnet::error::ConfigError("bad config");
    } catch (const bnet::error::Error&) {
        caughtAsError = true;
    }
    assert(caughtAsError);

    std::cout << "  PASSED" << std::endl;
}

void testErrorRegistry() {
    std::cout << "Testing ErrorRegistry..." << std::endl;

    auto& registry = bnet::ErrorRegistry::instance();

    // Create an exception_ptr
    std::exception_ptr timeout = std::make_exception_ptr(
        bnet::error::TimeoutError("test timeout", 30));

    // Test matching - should match self and all parent types
    assert(registry.matches(timeout, "bnet::error::TimeoutError"));
    assert(registry.matches(timeout, "bnet::error::NetworkError"));
    assert(registry.matches(timeout, "bnet::error::RuntimeError"));
    assert(registry.matches(timeout, "bnet::error::Error"));

    // Should not match sibling or unrelated types
    assert(!registry.matches(timeout, "bnet::error::ConnectionError"));
    assert(!registry.matches(timeout, "bnet::error::ValidationError"));

    // Test type name extraction
    assert(registry.getTypeName(timeout) == "bnet::error::TimeoutError");

    std::cout << "  PASSED" << std::endl;
}

void testActorParams() {
    std::cout << "Testing ActorParams..." << std::endl;

    bnet::ActorParams params({
        {"id", "robot_001"},
        {"address", "192.168.1.10"},
        {"port", "8080"},
        {"enabled", "true"},
        {"timeout", "30"}
    });

    // String access
    assert(params.get("id") == "robot_001");
    assert(params.getOr("missing", "default") == "default");
    assert(params.has("address"));
    assert(!params.has("missing"));

    // Integer access
    assert(params.getInt("port") == 8080);
    assert(params.getIntOr("missing", 100) == 100);

    // Bool access
    assert(params.getBool("enabled") == true);
    assert(params.getBoolOr("missing", false) == false);

    std::cout << "  PASSED" << std::endl;
}

void testToken() {
    std::cout << "Testing Token..." << std::endl;

    bnet::Token token;

    // Token starts empty - getActor should throw ActorNotFoundError
    bool threw = false;
    try {
        (void)token.getActor<warehouse::AMRActor>();
    } catch (const bnet::error::ActorNotFoundError& e) {
        threw = true;
        // Verify it's in the error hierarchy
        assert(dynamic_cast<const bnet::error::ResourceError*>(&e) != nullptr);
    }
    assert(threw);

    std::cout << "  PASSED" << std::endl;
}

void testRegistry() {
    std::cout << "Testing Registry..." << std::endl;

    auto& registry = bnet::Registry::instance();

    // Check that warehouse actors are registered
    assert(registry.hasActorType("user::AMR"));
    assert(registry.hasActorType("user::BinPickingStation"));
    assert(registry.hasActorType("user::PackingStation"));

    // Check that actions are registered
    assert(registry.hasAction("user::is_charged"));
    assert(registry.hasAction("user::charge"));
    assert(registry.hasAction("user::transport_bins"));
    assert(registry.hasAction("user::execute_order"));
    assert(registry.hasAction("user::pack"));
    assert(registry.hasAction("user::notify_done"));

    // Create an actor
    bnet::ActorParams amrParams({
        {"id", "amr_001"},
        {"Addr", "192.168.1.10:8080"}
    });
    auto amrActor = registry.createActor("user::AMR", amrParams);
    assert(amrActor != nullptr);

    // Check action info
    auto& actionInfo = registry.getActionInfo("user::transport_bins");
    assert(actionInfo.requiresToken == true);

    auto& actionInfoNoToken = registry.getActionInfo("user::is_charged");
    assert(actionInfoNoToken.requiresToken == false);

    std::cout << "  PASSED" << std::endl;
}

void testActorCreationAndActions() {
    std::cout << "Testing actor creation and action invocation..." << std::endl;

    auto& registry = bnet::Registry::instance();

    // Create AMR actor
    bnet::ActorParams params({
        {"id", "amr_test"},
        {"Addr", "127.0.0.1:8080"},
        {"metadata", "test_zone"}
    });
    auto actor = registry.createActor("user::AMR", params);

    // Invoke action without token
    auto result = registry.invokeAction("user::is_charged", *actor);
    assert(result.isSuccess() || result.isFailure());

    // Invoke action with token
    bnet::Token token;
    auto resultWithToken = registry.invokeAction("user::transport_bins", *actor, token);
    assert(resultWithToken.isSuccess());

    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "=== BehaviorNet API Tests ===" << std::endl;

    testActionResult();
    testErrorHierarchy();
    testErrorRegistry();
    testActorParams();
    testToken();
    testRegistry();
    testActorCreationAndActions();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
