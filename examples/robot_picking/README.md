# Robot Picking Example

This example demonstrates a BehaviorNet workflow with **user-defined actors** and actions. It simulates a warehouse robot picking system where a robot arm picks items from a conveyor belt.

## User-Defined Actors

### RobotActor
Represents a robot arm with actions:
- `move_to_position` - Move to a specific position
- `pick_item` - Pick up an item
- `place_item` - Place the item at destination

### ConveyorActor
Represents a conveyor belt with actions:
- `start` - Start the conveyor
- `stop` - Stop the conveyor
- `wait_for_item` - Wait until an item is at the pickup position

## Workflow Overview

```
[entry] -> [start_conveyor] -> [wait_for_item] -> [move_to_pickup]
    -> [pick_item] -> [move_to_dropoff] -> [place_item] -> [stop_conveyor] -> [exit]
```

1. Token enters at `entry` with a picking task
2. `start_conveyor` activates the conveyor belt
3. `wait_for_item` waits for an item to arrive at the pickup zone
4. `move_to_pickup` moves the robot arm to the pickup position
5. `pick_item` grabs the item
6. `move_to_dropoff` moves to the destination
7. `place_item` releases the item
8. `stop_conveyor` stops the conveyor
9. Token exits with completion status

## Files

- `config.json` - BehaviorNet configuration defining places and transitions
- `robot_actors.hpp` - User-defined C++ actors (RobotActor, ConveyorActor)
- `main.cpp` - C++ runner that loads config and executes the workflow
- `BUILD.bazel` - Build configuration
- `test.cpp` - Unit and integration tests

## Building

```bash
# From the behaviornet root directory

# Build the example
bazel build //examples/robot_picking:robot_picking_example

# Run the example
bazel run //examples/robot_picking:robot_picking_example

# Run the tests
bazel test //examples/robot_picking:robot_picking_test
```

## How to Create Your Own Actors

### Step 1: Define the Actor Class

```cpp
#include <bnet/Actor.hpp>
#include <bnet/ActionResult.hpp>
#include <bnet/Token.hpp>

class MyActor : public bnet::ActorBase
{
public:
    explicit MyActor(const std::string& name) : name_(name) {}

    // Action methods receive a token and return an ActionResult
    bnet::ActionResult myAction(bnet::Token& token)
    {
        // Read input from token
        auto input = token.getData("input");

        // Do something...

        // Store results back in token
        token.setData("result", "done");

        return bnet::ActionResult::success();
    }

private:
    std::string name_;
};
```

### Step 2: Register Actions with RuntimeController

```cpp
bnet::RuntimeController controller;
MyActor myActor("actor1");

controller.registerActionInvoker("my_action",
    [&myActor](bnet::ActorBase*, bnet::Token& token) {
        return myActor.myAction(token);
    });
```

### Step 3: Reference in Config

```json
{
  "actors": [
    {"id": "MyActor", "required_init_params": {}}
  ],
  "actions": [
    {"id": "my_action", "required_actors": ["MyActor"]}
  ],
  "places": [
    {
      "id": "do_something",
      "type": "action",
      "params": {"action_id": "my_action"}
    }
  ]
}
```

## Action Return Values

Actions should return appropriate `ActionResult` values:

- `ActionResult::success()` - Action completed successfully, token moves to `::success` subplace
- `ActionResult::failure("reason")` - Action failed, token moves to `::failure` subplace
- `ActionResult::inProgress()` - Action still running, will be polled again
- `ActionResult::error<MyError>("message")` - Unrecoverable error, token moves to `::error` subplace

## Token Data Flow

Tokens carry data through the workflow. Each action can read and write token data:

```cpp
// Read data
auto taskId = token.getDataOr<std::string>("task_id", "default");

// Write data
token.setData("pick_count", 5);
token.setData("status", nlohmann::json{{"success", true}});
```
