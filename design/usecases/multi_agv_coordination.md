# Multi-AGV Traffic Coordination

This use case models a fleet of Autonomous Guided Vehicles (AGVs) navigating a warehouse or factory floor with shared corridors, intersections, and restricted zones. The key challenge is **deadlock-free routing** when multiple AGVs compete for shared zone access.

## Overview

The Petri net approach excels here because:
- **Zones as resources**: Corridor segments and intersections become resource pools
- **Deadlock prevention**: Formal analysis detects circular wait conditions
- **Natural concurrency**: Multiple AGVs operate independently until resource contention

### Why BehaviorNet vs BehaviorTree.CPP

In BehaviorTree.CPP, this scenario requires:
- Complex blackboard with zone lock states
- Custom decorators for zone acquisition/release
- Manual deadlock avoidance logic
- Tight coupling between AGV trees

In BehaviorNet:
- Zones are `resource_pool` places
- Transitions require zone tokens to fire
- Deadlock detection is automatic via Petri net analysis
- AGVs are independent tokens

## Scenario

A manufacturing floor has:
- 10 AGVs transporting materials
- 4 loading docks (pickup points)
- 4 unloading stations (dropoff points)
- Shared corridors with capacity constraints
- 2 critical intersections (single AGV access)

The system must:
1. Assign AGVs to transport jobs
2. Route AGVs through corridors
3. Prevent collisions at intersections
4. Handle charging when battery is low
5. Recover gracefully from errors

## Actors

```cpp
#include <bnet/ActorInterface.hpp>

class AGVActor : public bnet::ActorInterface
{
public:
    AGVActor(bnet::ActorInitDict initDict)
        : m_agvAddr(initDict["Addr"])
        , m_agvId(initDict["id"])
    {
        assertValidConnection();
    }

    bnet::ActionResult navigateTo(bnet::Token const& token)
    {
        auto destination = token.getData<std::string>("destination");
        return sendNavigationCommand(destination);
    }

    bnet::ActionResult pickup()
    {
        return sendPickupCommand();
    }

    bnet::ActionResult dropoff()
    {
        return sendDropoffCommand();
    }

    bnet::ActionResult checkBattery()
    {
        if (getBatteryLevel() > 20)
            return bnet::ActionResult::SUCCESS;
        return bnet::ActionResult::FAILURE;
    }

    bnet::ActionResult charge()
    {
        return sendChargeCommand();
    }

    std::string getLocation() const { return m_currentLocation; }

private:
    void assertValidConnection();
    int getBatteryLevel();
    bnet::ActionResult sendNavigationCommand(std::string const& dest);
    bnet::ActionResult sendPickupCommand();
    bnet::ActionResult sendDropoffCommand();
    bnet::ActionResult sendChargeCommand();

    std::string m_agvAddr;
    std::string m_agvId;
    std::string m_currentLocation;
};

class ZoneActor : public bnet::ActorInterface
{
public:
    ZoneActor(bnet::ActorInitDict initDict)
        : m_zoneId(initDict["id"])
        , m_capacity(std::stoi(initDict["capacity"]))
    {
    }

    std::string getZoneId() const { return m_zoneId; }
    int getCapacity() const { return m_capacity; }

private:
    std::string m_zoneId;
    int m_capacity;
};

class TransportJobActor : public bnet::ActorInterface
{
public:
    TransportJobActor(bnet::ActorInitDict initDict)
        : m_jobId(initDict["job_id"])
        , m_pickupLocation(initDict["pickup"])
        , m_dropoffLocation(initDict["dropoff"])
        , m_priority(std::stoi(initDict.value("priority", "1")))
    {
    }

    std::string getPickupLocation() const { return m_pickupLocation; }
    std::string getDropoffLocation() const { return m_dropoffLocation; }
    int getPriority() const { return m_priority; }

private:
    std::string m_jobId;
    std::string m_pickupLocation;
    std::string m_dropoffLocation;
    int m_priority;
};

BNET_REGISTER_ACTOR(AGVActor, "AGV");
BNET_REGISTER_ACTOR(ZoneActor, "Zone");
BNET_REGISTER_ACTOR(TransportJobActor, "TransportJob");

BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(AGVActor, navigateTo, "navigate_to");
BNET_REGISTER_ACTOR_ACTION(AGVActor, pickup, "pickup");
BNET_REGISTER_ACTOR_ACTION(AGVActor, dropoff, "dropoff");
BNET_REGISTER_ACTOR_ACTION(AGVActor, checkBattery, "check_battery");
BNET_REGISTER_ACTOR_ACTION(AGVActor, charge, "charge");
```

## User Types Config

```json
{
    "actors": [
        {
            "id": "user::AGV",
            "required_init_params": {
                "id": { "type": "str" },
                "Addr": { "type": "str" }
            },
            "optional_init_params": {
                "home_zone": { "type": "str" }
            }
        },
        {
            "id": "user::Zone",
            "required_init_params": {
                "id": { "type": "str" },
                "capacity": { "type": "int" }
            },
            "optional_init_params": {}
        },
        {
            "id": "user::TransportJob",
            "required_init_params": {
                "job_id": { "type": "str" },
                "pickup": { "type": "str" },
                "dropoff": { "type": "str" }
            },
            "optional_init_params": {
                "priority": { "type": "int" }
            }
        }
    ],
    "actions": [
        {
            "id": "user::navigate_to",
            "required_actors": [ "user::AGV" ]
        },
        {
            "id": "user::pickup",
            "required_actors": [ "user::AGV" ]
        },
        {
            "id": "user::dropoff",
            "required_actors": [ "user::AGV" ]
        },
        {
            "id": "user::check_battery",
            "required_actors": [ "user::AGV" ]
        },
        {
            "id": "user::charge",
            "required_actors": [ "user::AGV" ]
        }
    ]
}
```

## Places Config

```json
{
    "places": [
        {
            "id": "job_entry",
            "type": "entrypoint",
            "params": {
                "new_actors": [ "user::TransportJob" ]
            }
        },
        {
            "id": "available_agvs",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::AGV",
                "initial_availability": 10
            }
        },
        {
            "id": "wait_for_agv",
            "type": "wait_with_timeout",
            "params": {
                "timeout_min": 5,
                "on_timeout": "error::no_agv_available"
            }
        },
        {
            "id": "intersection_alpha",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Zone",
                "initial_availability": 1,
                "description": "Critical intersection - single AGV access"
            }
        },
        {
            "id": "intersection_beta",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Zone",
                "initial_availability": 1,
                "description": "Critical intersection - single AGV access"
            }
        },
        {
            "id": "corridor_north",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Zone",
                "initial_availability": 3,
                "description": "North corridor - capacity 3 AGVs"
            }
        },
        {
            "id": "corridor_south",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Zone",
                "initial_availability": 3,
                "description": "South corridor - capacity 3 AGVs"
            }
        },
        {
            "id": "navigate_to_pickup",
            "type": "action",
            "params": {
                "action_id": "user::navigate_to",
                "retries": 2,
                "timeout_per_try_min": 5,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "execute_pickup",
            "type": "action",
            "params": {
                "action_id": "user::pickup",
                "retries": 2,
                "timeout_per_try_min": 3,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "navigate_to_dropoff",
            "type": "action",
            "params": {
                "action_id": "user::navigate_to",
                "retries": 2,
                "timeout_per_try_min": 5,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "execute_dropoff",
            "type": "action",
            "params": {
                "action_id": "user::dropoff",
                "retries": 2,
                "timeout_per_try_min": 3,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "check_agv_battery",
            "type": "action",
            "params": {
                "action_id": "user::check_battery",
                "retries": 0,
                "timeout_per_try_s": 5,
                "failure_as_error": false,
                "error_to_global_handler": true
            }
        },
        {
            "id": "charging_stations",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Zone",
                "initial_availability": 2,
                "description": "Charging station slots"
            }
        },
        {
            "id": "charge_agv",
            "type": "action",
            "params": {
                "action_id": "user::charge",
                "retries": 1,
                "timeout_per_try_min": 30,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "log_job_completion",
            "type": "exit_logger"
        }
    ]
}
```

## Transitions Config

```json
{
    "transitions": [
        {
            "id": "assign_agv_to_job",
            "from": [ "job_entry" ],
            "to": [ "wait_for_agv" ]
        },
        {
            "id": "agv_accepts_job",
            "from": [ "wait_for_agv", "available_agvs" ],
            "to": [ "navigate_to_pickup" ],
            "description": "AGV token merges with job token"
        },
        {
            "id": "acquire_corridor_north_for_pickup",
            "from": [ "navigate_to_pickup::success", "corridor_north" ],
            "to": [ "execute_pickup" ],
            "priority": 2,
            "description": "Route via north corridor if available"
        },
        {
            "id": "acquire_corridor_south_for_pickup",
            "from": [ "navigate_to_pickup::success", "corridor_south" ],
            "to": [ "execute_pickup" ],
            "priority": 1,
            "description": "Fallback to south corridor"
        },
        {
            "id": "pickup_complete_release_corridor",
            "from": [ "execute_pickup::success" ],
            "to": [
                { "to": "navigate_to_dropoff" },
                { "to": "corridor_north", "token_filter": "user::Zone", "condition": "zone_id == 'corridor_north'" },
                { "to": "corridor_south", "token_filter": "user::Zone", "condition": "zone_id == 'corridor_south'" }
            ]
        },
        {
            "id": "acquire_intersection_alpha",
            "from": [ "navigate_to_dropoff::success", "intersection_alpha" ],
            "to": [ "execute_dropoff" ],
            "description": "Must acquire intersection before entering dropoff zone"
        },
        {
            "id": "dropoff_complete",
            "from": [ "execute_dropoff::success" ],
            "to": [
                { "to": "log_job_completion", "token_filter": "user::TransportJob" },
                { "to": "check_agv_battery", "token_filter": "user::AGV" },
                { "to": "intersection_alpha", "token_filter": "user::Zone" }
            ]
        },
        {
            "id": "agv_battery_ok",
            "from": [ "check_agv_battery::success" ],
            "to": [ "available_agvs" ]
        },
        {
            "id": "agv_needs_charging",
            "from": [ "check_agv_battery::failure", "charging_stations" ],
            "to": [ "charge_agv" ]
        },
        {
            "id": "charging_complete",
            "from": [ "charge_agv::success" ],
            "to": [
                { "to": "available_agvs", "token_filter": "user::AGV" },
                { "to": "charging_stations", "token_filter": "user::Zone" }
            ]
        }
    ]
}
```

## Deadlock Prevention via Petri Net Analysis

This use case demonstrates BehaviorNet's deadlock detection capabilities:

### Potential Deadlock Scenario
Without proper analysis, the following could occur:
1. AGV-1 holds `corridor_north`, waiting for `intersection_alpha`
2. AGV-2 holds `intersection_alpha`, waiting for `corridor_north`
3. Both AGVs blocked indefinitely

### BehaviorNet's Solution
Static analysis at config load detects:
- Circular wait conditions in the zone acquisition graph
- Suggests priority ordering or zone acquisition sequences
- Warns about potential starvation of lower-priority routes

```json
{
    "deadlock_analysis": {
        "enabled": true,
        "on_potential_deadlock": "warn",
        "on_guaranteed_deadlock": "error",
        "zone_acquisition_order": [
            "corridor_north",
            "corridor_south",
            "intersection_alpha",
            "intersection_beta"
        ]
    }
}
```

## Error Handling

```json
{
    "global_error_handler": {
        "places": [
            {
                "id": "emergency_stop",
                "type": "action",
                "params": {
                    "action_id": "system::emergency_stop_agv",
                    "retries": 3,
                    "timeout_per_try_s": 10
                }
            },
            {
                "id": "notify_fleet_manager",
                "type": "action",
                "params": {
                    "action_id": "system::notify_operator",
                    "retries": 3,
                    "timeout_per_try_s": 30
                }
            },
            {
                "id": "release_held_zones",
                "type": "action",
                "params": {
                    "action_id": "system::force_release_zones",
                    "description": "Release any zones held by errored AGV to prevent fleet-wide deadlock"
                }
            }
        ],
        "mapping": [
            {
                "error_id_filter": "error::navigation_failure",
                "actor_filter": "user::AGV",
                "destination": "emergency_stop"
            },
            {
                "error_id_filter": "error::no_agv_available",
                "destination": "notify_fleet_manager"
            },
            {
                "error_id_filter": "*",
                "actor_filter": "user::AGV",
                "destination": "release_held_zones"
            }
        ]
    }
}
```

## Token Structures

AGV tokens:
```json
{
  "AGV": {
    "id": "agv_003",
    "Addr": "192.168.1.103:8080",
    "home_zone": "charging_bay_1"
  }
}
```

Zone tokens:
```json
{
  "Zone": {
    "id": "intersection_alpha",
    "capacity": 1
  }
}
```

Transport job tokens:
```json
{
  "TransportJob": {
    "job_id": "job_12345",
    "pickup": "dock_a",
    "dropoff": "station_3",
    "priority": 2
  }
}
```

## Key Insights

1. **Zones as tokens** - Physical zones become first-class entities that can be acquired/released
2. **Natural deadlock modeling** - Circular waits are visible in the Petri net structure
3. **Priority routing** - Transition priorities enable preferred path selection
4. **Clean error recovery** - Zone tokens can be forcibly returned to pools on AGV failure
5. **Scalability** - Adding AGVs = adding tokens, adding zones = adding resource pools
