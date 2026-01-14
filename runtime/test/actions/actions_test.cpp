// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/Token.hpp>
#include <bnet/actions/ConditionAction.hpp>
#include <bnet/actions/DelayAction.hpp>

#include <cassert>
#include <iostream>
#include <thread>

void testNoOpAction()
{
    std::cout << "Testing NoOpAction..." << std::endl;

    bnet::actions::NoOpAction action;
    bnet::Token token;

    auto result = action.execute(token);
    assert(result.isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testFailAction()
{
    std::cout << "Testing FailAction..." << std::endl;

    bnet::actions::FailAction action("Test failure message");
    bnet::Token token;

    auto result = action.execute(token);
    assert(result.isFailure());
    assert(token.getData("failure_message") == "Test failure message");

    std::cout << "  PASSED" << std::endl;
}

void testErrorAction()
{
    std::cout << "Testing ErrorAction..." << std::endl;

    bnet::actions::ErrorAction action("Test error message");
    bnet::Token token;

    auto result = action.execute(token);
    assert(result.isError());
    assert(token.getData("error_message") == "Test error message");

    std::cout << "  PASSED" << std::endl;
}

void testDelayAction()
{
    std::cout << "Testing DelayAction..." << std::endl;

    bnet::actions::DelayAction action(std::chrono::milliseconds(50));
    bnet::Token token;

    // First call should return in_progress
    auto result1 = action.execute(token);
    assert(result1.isInProgress());
    assert(token.hasData("_delay_start"));

    // Wait for delay
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Second call should return success
    auto result2 = action.execute(token);
    assert(result2.isSuccess());
    assert(!token.hasData("_delay_start"));  // Cleaned up

    std::cout << "  PASSED" << std::endl;
}

void testDelayActionWithTokenData()
{
    std::cout << "Testing DelayAction with token data..." << std::endl;

    bnet::actions::DelayAction action(std::chrono::milliseconds(1000));  // Long default
    bnet::Token token;
    token.setData("delay_ms", 30);  // Short override

    auto result1 = action.execute(token);
    assert(result1.isInProgress());

    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    auto result2 = action.execute(token);
    assert(result2.isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testConditionAction()
{
    std::cout << "Testing ConditionAction..." << std::endl;

    bnet::actions::ConditionAction action;

    // Test with boolean
    bnet::Token token1;
    token1.setData("condition", true);
    auto result1 = action.execute(token1);
    assert(result1.isSuccess());
    assert(token1.getData("condition_result") == true);

    bnet::Token token2;
    token2.setData("condition", false);
    auto result2 = action.execute(token2);
    assert(result2.isFailure());
    assert(token2.getData("condition_result") == false);

    // Test with number
    bnet::Token token3;
    token3.setData("condition", 1);
    auto result3 = action.execute(token3);
    assert(result3.isSuccess());

    bnet::Token token4;
    token4.setData("condition", 0);
    auto result4 = action.execute(token4);
    assert(result4.isFailure());

    std::cout << "  PASSED" << std::endl;
}

void testConditionActionWithPredicate()
{
    std::cout << "Testing ConditionAction with predicate..." << std::endl;

    auto action = bnet::actions::ConditionAction([](const bnet::Token& t) -> bool
    {
        return t.hasData("required_key");
    });

    bnet::Token token1;
    auto result1 = action.execute(token1);
    assert(result1.isFailure());

    bnet::Token token2;
    token2.setData("required_key", "present");
    auto result2 = action.execute(token2);
    assert(result2.isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testConditionActionCheckDataKey()
{
    std::cout << "Testing ConditionAction::checkDataKey..." << std::endl;

    auto action = bnet::actions::ConditionAction::checkDataKey("flag");

    bnet::Token token1;
    token1.setData("flag", true);
    assert(action.execute(token1).isSuccess());

    bnet::Token token2;
    token2.setData("flag", false);
    assert(action.execute(token2).isFailure());

    bnet::Token token3;
    token3.setData("flag", "yes");
    assert(action.execute(token3).isSuccess());

    bnet::Token token4;
    token4.setData("flag", "");
    assert(action.execute(token4).isFailure());

    std::cout << "  PASSED" << std::endl;
}

void testConditionActionCheckEquals()
{
    std::cout << "Testing ConditionAction::checkEquals..." << std::endl;

    auto action = bnet::actions::ConditionAction::checkEquals("status", "active");

    bnet::Token token1;
    token1.setData("status", "active");
    assert(action.execute(token1).isSuccess());

    bnet::Token token2;
    token2.setData("status", "inactive");
    assert(action.execute(token2).isFailure());

    bnet::Token token3;  // Missing key
    assert(action.execute(token3).isFailure());

    std::cout << "  PASSED" << std::endl;
}

void testConditionActionCheckExists()
{
    std::cout << "Testing ConditionAction::checkExists..." << std::endl;

    auto action = bnet::actions::ConditionAction::checkExists("data");

    bnet::Token token1;
    assert(action.execute(token1).isFailure());

    bnet::Token token2;
    token2.setData("data", nullptr);
    assert(action.execute(token2).isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testConditionActionNumericComparison()
{
    std::cout << "Testing ConditionAction numeric comparisons..." << std::endl;

    auto gtAction = bnet::actions::ConditionAction::checkGreaterThan("value", 10);
    auto ltAction = bnet::actions::ConditionAction::checkLessThan("value", 10);

    bnet::Token token1;
    token1.setData("value", 15);
    assert(gtAction.execute(token1).isSuccess());
    assert(ltAction.execute(token1).isFailure());

    bnet::Token token2;
    token2.setData("value", 5);
    assert(gtAction.execute(token2).isFailure());
    assert(ltAction.execute(token2).isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testWaitForConditionAction()
{
    std::cout << "Testing WaitForConditionAction..." << std::endl;

    bool conditionMet = false;
    bnet::actions::WaitForConditionAction action(
        [&conditionMet](const bnet::Token&) { return conditionMet; },
        std::chrono::milliseconds(100));

    bnet::Token token;

    // Should return in_progress initially
    auto result1 = action.execute(token);
    assert(result1.isInProgress());

    // Set condition and check again
    conditionMet = true;
    auto result2 = action.execute(token);
    assert(result2.isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testWaitForConditionTimeout()
{
    std::cout << "Testing WaitForConditionAction timeout..." << std::endl;

    bnet::actions::WaitForConditionAction action(
        [](const bnet::Token&) { return false; },  // Never true
        std::chrono::milliseconds(30));

    bnet::Token token;

    auto result1 = action.execute(token);
    assert(result1.isInProgress());

    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    auto result2 = action.execute(token);
    assert(result2.isFailure());

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Actions Tests ===" << std::endl;

    testNoOpAction();
    testFailAction();
    testErrorAction();
    testDelayAction();
    testDelayActionWithTokenData();
    testConditionAction();
    testConditionActionWithPredicate();
    testConditionActionCheckDataKey();
    testConditionActionCheckEquals();
    testConditionActionCheckExists();
    testConditionActionNumericComparison();
    testWaitForConditionAction();
    testWaitForConditionTimeout();

    std::cout << "\n=== All actions tests passed ===" << std::endl;
    return 0;
}
