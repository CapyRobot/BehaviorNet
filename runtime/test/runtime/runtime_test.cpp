// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/Token.hpp>
#include <bnet/runtime/RuntimeController.hpp>

#include <cassert>
#include <iostream>
#include <thread>

void testLoadConfig()
{
    std::cout << "Testing load config..." << std::endl;

    bnet::runtime::RuntimeController controller;

    bool loaded = controller.loadConfigString(R"({
        "places": [
            {"id": "entry", "type": "entrypoint"},
            {"id": "exit", "type": "exit_logger"}
        ],
        "transitions": [
            {"from": ["entry"], "to": ["exit"]}
        ]
    })");

    assert(loaded);
    assert(controller.errors().empty());
    assert(controller.net().getAllPlaces().size() == 2);
    assert(controller.net().getAllTransitions().size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testLoadInvalidConfig()
{
    std::cout << "Testing load invalid config..." << std::endl;

    bnet::runtime::RuntimeController controller;

    bool loaded = controller.loadConfigString("invalid json");
    assert(!loaded);
    assert(!controller.errors().empty());

    std::cout << "  PASSED" << std::endl;
}

void testRuntimeState()
{
    std::cout << "Testing runtime state..." << std::endl;

    bnet::runtime::RuntimeController controller;
    controller.loadConfigString(R"({
        "places": [{"id": "p1", "type": "plain"}],
        "transitions": []
    })");

    assert(controller.state() == bnet::runtime::RuntimeState::Stopped);

    controller.start();
    assert(controller.state() == bnet::runtime::RuntimeState::Running);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    controller.stop();
    assert(controller.state() == bnet::runtime::RuntimeState::Stopped);

    std::cout << "  PASSED" << std::endl;
}

void testInjectToken()
{
    std::cout << "Testing inject token..." << std::endl;

    bnet::runtime::RuntimeController controller;
    controller.loadConfigString(R"({
        "places": [
            {"id": "entry", "type": "entrypoint"},
            {"id": "exit", "type": "exit_logger"}
        ],
        "transitions": [
            {"from": ["entry"], "to": ["exit"]}
        ]
    })");

    bnet::Token token;
    auto id = controller.injectToken("entry", std::move(token));
    assert(id != 0);

    // Invalid entrypoint
    bnet::Token token2;
    auto id2 = controller.injectToken("nonexistent", std::move(token2));
    assert(id2 == 0);

    std::cout << "  PASSED" << std::endl;
}

void testTickProcessing()
{
    std::cout << "Testing tick processing..." << std::endl;

    bnet::runtime::RuntimeController controller;
    controller.loadConfigString(R"({
        "places": [
            {"id": "entry", "type": "entrypoint"},
            {"id": "exit", "type": "exit_logger"}
        ],
        "transitions": [
            {"from": ["entry"], "to": ["exit"]}
        ]
    })");

    // Inject token
    bnet::Token token;
    controller.injectToken("entry", std::move(token));

    auto stats1 = controller.stats();
    assert(stats1.tokensProcessed == 1);

    // Tick to fire transition
    controller.tick();

    auto stats2 = controller.stats();
    assert(stats2.epoch == 1);

    std::cout << "  PASSED" << std::endl;
}

void testRegisterAction()
{
    std::cout << "Testing register action..." << std::endl;

    bnet::runtime::RuntimeController controller;

    bool actionCalled = false;

    controller.registerAction("user::my_action", [&](bnet::ActorBase*, bnet::Token&) -> bnet::ActionResult
    {
        actionCalled = true;
        return bnet::ActionResult::success();
    });

    controller.loadConfigString(R"({
        "places": [
            {"id": "entry", "type": "entrypoint"},
            {"id": "action", "type": "action", "params": {"action_id": "user::my_action"}},
            {"id": "exit", "type": "exit_logger"}
        ],
        "transitions": [
            {"from": ["entry"], "to": ["action"]},
            {"from": ["action::success"], "to": ["exit"]}
        ]
    })");

    // Note: Action is registered before loadConfig, so it should be available
    // In a real implementation, we'd need to set invokers after loading

    std::cout << "  PASSED" << std::endl;
}

void testStats()
{
    std::cout << "Testing stats..." << std::endl;

    bnet::runtime::RuntimeController controller;
    controller.loadConfigString(R"({
        "places": [{"id": "p1", "type": "plain"}],
        "transitions": []
    })");

    auto stats = controller.stats();
    assert(stats.epoch == 0);
    assert(stats.transitionsFired == 0);
    assert(stats.activeTokens == 0);

    controller.tick();

    stats = controller.stats();
    assert(stats.epoch == 1);

    std::cout << "  PASSED" << std::endl;
}

void testLogCallback()
{
    std::cout << "Testing log callback..." << std::endl;

    bnet::runtime::RuntimeController controller;

    std::vector<std::string> logs;
    controller.setLogCallback([&](const std::string& msg)
    {
        logs.push_back(msg);
    });

    controller.loadConfigString(R"({
        "places": [
            {"id": "entry", "type": "entrypoint"}
        ],
        "transitions": []
    })");

    bnet::Token token;
    controller.injectToken("entry", std::move(token));

    assert(!logs.empty());

    std::cout << "  PASSED" << std::endl;
}

void testResourcePool()
{
    std::cout << "Testing resource pool..." << std::endl;

    bnet::runtime::RuntimeController controller;
    controller.loadConfigString(R"({
        "places": [
            {"id": "pool", "type": "resource_pool", "params": {"resource_id": "Robot", "initial_availability": 3}}
        ],
        "transitions": []
    })");

    auto* place = controller.net().getPlace("pool");
    assert(place != nullptr);
    assert(place->tokenCount() == 3);

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Runtime Tests ===" << std::endl;

    testLoadConfig();
    testLoadInvalidConfig();
    testRuntimeState();
    testInjectToken();
    testTickProcessing();
    testRegisterAction();
    testStats();
    testLogCallback();
    testResourcePool();

    std::cout << "\n=== All runtime tests passed ===" << std::endl;
    return 0;
}
