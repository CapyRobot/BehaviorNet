// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/bnet.hpp>
#include <bnet/core/Arc.hpp>
#include <bnet/core/Net.hpp>
#include <bnet/core/Place.hpp>
#include <bnet/core/TokenQueue.hpp>
#include <bnet/core/Transition.hpp>

#include <cassert>
#include <iostream>
#include <thread>

void testTokenQueue()
{
    std::cout << "Testing TokenQueue..." << std::endl;

    bnet::core::TokenQueue queue;

    // Test empty queue
    assert(queue.empty());
    assert(queue.size() == 0);
    assert(queue.availableCount() == 0);
    assert(queue.peek() == nullptr);
    assert(!queue.pop().has_value());

    // Test push and pop
    bnet::Token token1;
    auto id1 = queue.push(std::move(token1));
    assert(!queue.empty());
    assert(queue.size() == 1);
    assert(queue.availableCount() == 1);
    assert(queue.peek() != nullptr);

    bnet::Token token2;
    auto id2 = queue.push(std::move(token2));
    assert(queue.size() == 2);

    // FIFO order - first in, first out
    auto popped = queue.pop();
    assert(popped.has_value());
    assert(popped->first == id1);
    assert(queue.size() == 1);

    // Test lock/unlock
    bnet::Token token3;
    auto id3 = queue.push(std::move(token3));
    queue.lock(id2);
    assert(queue.availableCount() == 1);  // Only token3 available

    auto poppedLocked = queue.pop();
    assert(poppedLocked.has_value());
    assert(poppedLocked->first == id3);  // Skipped locked token

    queue.unlock(id2);
    assert(queue.availableCount() == 1);

    // Test remove by ID
    auto removed = queue.remove(id2);
    assert(removed.has_value());
    assert(queue.empty());

    // Test getByWaitingTime
    bnet::Token t1, t2, t3;
    auto tid1 = queue.push(std::move(t1));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    queue.push(std::move(t2));
    queue.push(std::move(t3));

    auto byTime = queue.getByWaitingTime();
    assert(byTime.size() == 3);
    assert(byTime[0] == tid1);  // Oldest first

    std::cout << "  PASSED" << std::endl;
}

void testArc()
{
    std::cout << "Testing Arc..." << std::endl;

    bnet::core::Arc inputArc("place1", "trans1", bnet::core::ArcDirection::PlaceToTransition);
    assert(inputArc.placeId() == "place1");
    assert(inputArc.transitionId() == "trans1");
    assert(inputArc.direction() == bnet::core::ArcDirection::PlaceToTransition);
    assert(!inputArc.tokenFilter().has_value());
    assert(inputArc.weight() == 1);

    inputArc.setTokenFilter("user::Robot");
    assert(inputArc.tokenFilter().has_value());
    assert(*inputArc.tokenFilter() == "user::Robot");

    inputArc.setWeight(2);
    assert(inputArc.weight() == 2);

    bnet::core::Arc outputArc("place2", "trans1", bnet::core::ArcDirection::TransitionToPlace);
    assert(outputArc.direction() == bnet::core::ArcDirection::TransitionToPlace);

    std::cout << "  PASSED" << std::endl;
}

void testTransition()
{
    std::cout << "Testing Transition..." << std::endl;

    bnet::core::Transition trans("t1");
    assert(trans.id() == "t1");
    assert(trans.priority() == 1);  // Default
    assert(trans.lastFiredEpoch() == 0);
    assert(trans.isAutoTrigger());  // Default

    trans.setPriority(5);
    assert(trans.priority() == 5);

    trans.setLastFiredEpoch(100);
    assert(trans.lastFiredEpoch() == 100);

    trans.setAutoTrigger(false);
    assert(!trans.isAutoTrigger());

    // Test arc management
    trans.addInputArc(bnet::core::Arc("p1", "t1", bnet::core::ArcDirection::PlaceToTransition));
    trans.addInputArc(bnet::core::Arc("p2", "t1", bnet::core::ArcDirection::PlaceToTransition));
    assert(trans.inputArcs().size() == 2);

    trans.addOutputArc(bnet::core::Arc("p3", "t1", bnet::core::ArcDirection::TransitionToPlace));
    assert(trans.outputArcs().size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testPlace()
{
    std::cout << "Testing Place..." << std::endl;

    bnet::core::Place place("p1");
    assert(place.id() == "p1");
    assert(!place.capacity().has_value());
    assert(place.requiredActors().empty());
    assert(place.tokenCount() == 0);
    assert(place.canAcceptToken());
    assert(!place.hasAvailableToken());
    assert(!place.hasSubplaces());

    // Test capacity
    place.setCapacity(2);
    assert(place.capacity().has_value());
    assert(*place.capacity() == 2);

    // Test required actors
    place.setRequiredActors({"user::Robot", "user::Order"});
    assert(place.requiredActors().size() == 2);

    // Test token operations
    bnet::Token token1;
    place.addToken(std::move(token1));
    assert(place.tokenCount() == 1);
    assert(place.hasAvailableToken());
    assert(place.canAcceptToken());

    bnet::Token token2;
    auto id2 = place.addToken(std::move(token2));
    assert(place.tokenCount() == 2);
    assert(!place.canAcceptToken());  // At capacity

    // Test capacity enforcement
    bool threwOnCapacity = false;
    try
    {
        bnet::Token token3;
        place.addToken(std::move(token3));
    }
    catch (const bnet::error::ResourceError&)
    {
        threwOnCapacity = true;
    }
    assert(threwOnCapacity);

    // Test remove
    auto removed = place.removeToken();
    assert(removed.has_value());
    assert(place.tokenCount() == 1);

    // Test remove by ID
    auto removedById = place.removeToken(id2);
    assert(removedById.has_value());
    assert(place.tokenCount() == 0);

    std::cout << "  PASSED" << std::endl;
}

void testPlaceSubplaces()
{
    std::cout << "Testing Place subplaces..." << std::endl;

    bnet::core::Place place("action_place");
    assert(!place.hasSubplaces());

    place.enableSubplaces();
    assert(place.hasSubplaces());

    // Add tokens to different subplaces
    bnet::Token t1, t2, t3;
    place.subplace(bnet::core::Subplace::Main).push(std::move(t1));
    place.subplace(bnet::core::Subplace::Success).push(std::move(t2));
    place.subplace(bnet::core::Subplace::Error).push(std::move(t3));

    assert(place.subplace(bnet::core::Subplace::Main).size() == 1);
    assert(place.subplace(bnet::core::Subplace::Success).size() == 1);
    assert(place.subplace(bnet::core::Subplace::Error).size() == 1);
    assert(place.subplace(bnet::core::Subplace::Failure).size() == 0);

    std::cout << "  PASSED" << std::endl;
}

void testSubplaceParsing()
{
    std::cout << "Testing subplace parsing..." << std::endl;

    auto [id1, sub1] = bnet::core::parseSubplace("my_place");
    assert(id1 == "my_place");
    assert(sub1 == bnet::core::Subplace::None);

    auto [id2, sub2] = bnet::core::parseSubplace("my_place::success");
    assert(id2 == "my_place");
    assert(sub2 == bnet::core::Subplace::Success);

    auto [id3, sub3] = bnet::core::parseSubplace("action::failure");
    assert(id3 == "action");
    assert(sub3 == bnet::core::Subplace::Failure);

    auto [id4, sub4] = bnet::core::parseSubplace("place::in_execution");
    assert(id4 == "place");
    assert(sub4 == bnet::core::Subplace::InExecution);

    std::cout << "  PASSED" << std::endl;
}

void testNet()
{
    std::cout << "Testing Net..." << std::endl;

    bnet::core::Net net;

    // Add places
    net.addPlace(std::make_unique<bnet::core::Place>("p1"));
    net.addPlace(std::make_unique<bnet::core::Place>("p2"));
    net.addPlace(std::make_unique<bnet::core::Place>("p3"));

    assert(net.getPlace("p1") != nullptr);
    assert(net.getPlace("p2") != nullptr);
    assert(net.getPlace("nonexistent") == nullptr);

    // Add transition
    bnet::core::Transition t1("t1");
    t1.addInputArc(bnet::core::Arc("p1", "t1", bnet::core::ArcDirection::PlaceToTransition));
    t1.addOutputArc(bnet::core::Arc("p2", "t1", bnet::core::ArcDirection::TransitionToPlace));
    net.addTransition(std::move(t1));

    assert(net.getTransition("t1") != nullptr);

    // Test getAllPlaces/getAllTransitions
    assert(net.getAllPlaces().size() == 3);
    assert(net.getAllTransitions().size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testNetFiring()
{
    std::cout << "Testing Net firing..." << std::endl;

    bnet::core::Net net;

    // Create simple net: p1 -> t1 -> p2
    net.addPlace(std::make_unique<bnet::core::Place>("p1"));
    net.addPlace(std::make_unique<bnet::core::Place>("p2"));

    bnet::core::Transition t1("t1");
    t1.addInputArc(bnet::core::Arc("p1", "t1", bnet::core::ArcDirection::PlaceToTransition));
    t1.addOutputArc(bnet::core::Arc("p2", "t1", bnet::core::ArcDirection::TransitionToPlace));
    net.addTransition(std::move(t1));

    auto* trans = net.getTransition("t1");
    auto* place1 = net.getPlace("p1");
    auto* place2 = net.getPlace("p2");

    // Initially, transition is not enabled (no tokens)
    assert(!net.isEnabled(*trans));

    // Add token to p1
    bnet::Token token;
    place1->addToken(std::move(token));
    assert(place1->tokenCount() == 1);

    // Now transition should be enabled
    assert(net.isEnabled(*trans));

    // Fire the transition
    auto result = net.fire(*trans, 1);
    assert(result.success);
    assert(place1->tokenCount() == 0);
    assert(place2->tokenCount() == 1);
    assert(trans->lastFiredEpoch() == 1);

    // Transition should no longer be enabled
    assert(!net.isEnabled(*trans));

    std::cout << "  PASSED" << std::endl;
}

void testNetPriority()
{
    std::cout << "Testing Net transition priority..." << std::endl;

    bnet::core::Net net;

    net.addPlace(std::make_unique<bnet::core::Place>("p1"));
    net.addPlace(std::make_unique<bnet::core::Place>("p2"));
    net.addPlace(std::make_unique<bnet::core::Place>("p3"));

    // Two transitions from p1
    bnet::core::Transition t1("t1");
    t1.setPriority(1);
    t1.addInputArc(bnet::core::Arc("p1", "t1", bnet::core::ArcDirection::PlaceToTransition));
    t1.addOutputArc(bnet::core::Arc("p2", "t1", bnet::core::ArcDirection::TransitionToPlace));
    net.addTransition(std::move(t1));

    bnet::core::Transition t2("t2");
    t2.setPriority(5);  // Higher priority
    t2.addInputArc(bnet::core::Arc("p1", "t2", bnet::core::ArcDirection::PlaceToTransition));
    t2.addOutputArc(bnet::core::Arc("p3", "t2", bnet::core::ArcDirection::TransitionToPlace));
    net.addTransition(std::move(t2));

    // Add token
    net.getPlace("p1")->addToken(bnet::Token{});

    // Get transitions by priority
    auto byPriority = net.getTransitionsByPriority();
    assert(byPriority.size() == 2);
    assert(byPriority[0]->id() == "t2");  // Higher priority first
    assert(byPriority[1]->id() == "t1");

    // Get enabled transitions
    auto enabled = net.getEnabledTransitions();
    assert(enabled.size() == 2);  // Both enabled

    std::cout << "  PASSED" << std::endl;
}

void testNetMultipleInputs()
{
    std::cout << "Testing Net with multiple input places..." << std::endl;

    bnet::core::Net net;

    net.addPlace(std::make_unique<bnet::core::Place>("p1"));
    net.addPlace(std::make_unique<bnet::core::Place>("p2"));
    net.addPlace(std::make_unique<bnet::core::Place>("p3"));

    // Transition with two inputs
    bnet::core::Transition t1("t1");
    t1.addInputArc(bnet::core::Arc("p1", "t1", bnet::core::ArcDirection::PlaceToTransition));
    t1.addInputArc(bnet::core::Arc("p2", "t1", bnet::core::ArcDirection::PlaceToTransition));
    t1.addOutputArc(bnet::core::Arc("p3", "t1", bnet::core::ArcDirection::TransitionToPlace));
    net.addTransition(std::move(t1));

    auto* trans = net.getTransition("t1");

    // Not enabled with no tokens
    assert(!net.isEnabled(*trans));

    // Add token to p1 only - still not enabled
    net.getPlace("p1")->addToken(bnet::Token{});
    assert(!net.isEnabled(*trans));

    // Add token to p2 - now enabled
    net.getPlace("p2")->addToken(bnet::Token{});
    assert(net.isEnabled(*trans));

    // Fire
    auto result = net.fire(*trans, 1);
    assert(result.success);
    assert(net.getPlace("p1")->tokenCount() == 0);
    assert(net.getPlace("p2")->tokenCount() == 0);
    assert(net.getPlace("p3")->tokenCount() == 1);

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Core Tests ===" << std::endl;

    testTokenQueue();
    testArc();
    testTransition();
    testPlace();
    testPlaceSubplaces();
    testSubplaceParsing();
    testNet();
    testNetFiring();
    testNetPriority();
    testNetMultipleInputs();

    std::cout << "\n=== All core tests passed ===" << std::endl;
    return 0;
}
