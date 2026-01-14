# Autonomous eCommerce Warehouse

This use case models an autonomous warehouse system using Petri nets, based on the example from [capybot.com](https://capybot.com/2023/03/03/pnet_example/). The system coordinates Autonomous Mobile Robots (AMRs), bin-picking stations, and packing stations to fulfill orders.

## Overview

The warehouse automation system handles two main workflows:

1. **Order Execution** - AMRs transport bins from the warehouse to bin-picking stations, which extract items, then to packing stations for fulfillment
2. **AMR Charging** - AMRs autonomously route to charging stations when battery is low

The key insight is that tokens carry entity information, making it trivial to scale: "Adding more robots is as trivial as adding a single token."

## Actors

```cpp
#include <bnet/ActorInterface.hpp>

class AMRActor : public bnet::ActorInterface
{
public:
    AMRActor(bnet::ActorInitDict initDict)
        : m_amrAddr(initDict["Addr"])
        , m_amrId(initDict["id"])
    {
        assertValidConnection();
    }

    bnet::ActionResult transportBins(bnet::Token const& token)
    {
        // Transport bins to destination
        auto destination = token.getActor<StationActor>().getLocation();
        return sendTransportCommand(destination);
    }

    bnet::ActionResult charge()
    {
        // Navigate to charger and charge
        return sendChargeCommand();
    }

    bnet::ActionResult isCharged()
    {
        // Check battery level
        if (getBatteryLevel() > 80)
            return bnet::ActionResult::SUCCESS;
        return bnet::ActionResult::FAILURE;
    }

private:
    void assertValidConnection();
    int getBatteryLevel();
    bnet::ActionResult sendTransportCommand(std::string const& dest);
    bnet::ActionResult sendChargeCommand();

    std::string m_amrAddr;
    std::string m_amrId;
};

class BinPickingStationActor : public bnet::ActorInterface
{
public:
    BinPickingStationActor(bnet::ActorInitDict initDict)
        : m_stationAddr(initDict["Addr"])
        , m_stationId(initDict["id"])
    {
    }

    bnet::ActionResult executeOrder(bnet::Token const& token)
    {
        // Execute picking operation for order
        auto orderId = token.getData<std::string>("order_id");
        return sendPickCommand(orderId);
    }

private:
    bnet::ActionResult sendPickCommand(std::string const& orderId);
    std::string m_stationAddr;
    std::string m_stationId;
};

class PackingStationActor : public bnet::ActorInterface
{
public:
    PackingStationActor(bnet::ActorInitDict initDict)
        : m_stationAddr(initDict["Addr"])
        , m_stationId(initDict["id"])
    {
    }

    bnet::ActionResult pack()
    {
        return sendPackCommand();
    }

    bnet::ActionResult notifyDone()
    {
        return sendNotification();
    }

private:
    bnet::ActionResult sendPackCommand();
    bnet::ActionResult sendNotification();
    std::string m_stationAddr;
    std::string m_stationId;
};

BNET_REGISTER_ACTOR(AMRActor, "AMR");
BNET_REGISTER_ACTOR(BinPickingStationActor, "BinPickingStation");
BNET_REGISTER_ACTOR(PackingStationActor, "PackingStation");

BNET_REGISTER_ACTOR_ACTION(AMRActor, isCharged, "is_charged");
BNET_REGISTER_ACTOR_ACTION(AMRActor, charge, "charge");
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(AMRActor, transportBins, "transport_bins");
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(BinPickingStationActor, executeOrder, "execute_order");
BNET_REGISTER_ACTOR_ACTION(PackingStationActor, pack, "pack");
BNET_REGISTER_ACTOR_ACTION(PackingStationActor, notifyDone, "notify_done");
```

## User Types Config

```json
{
    "actors": [
        {
            "id": "user::AMR",
            "required_init_params": {
                "id": { "type": "str" },
                "Addr": { "type": "str" }
            },
            "optional_init_params": {
                "metadata": { "type": "str" }
            }
        },
        {
            "id": "user::BinPickingStation",
            "required_init_params": {
                "id": { "type": "str" },
                "Addr": { "type": "str" }
            },
            "optional_init_params": {}
        },
        {
            "id": "user::PackingStation",
            "required_init_params": {
                "id": { "type": "str" },
                "Addr": { "type": "str" }
            },
            "optional_init_params": {}
        }
    ],
    "actions": [
        {
            "id": "user::is_charged",
            "required_actors": [ "user::AMR" ]
        },
        {
            "id": "user::charge",
            "required_actors": [ "user::AMR" ]
        },
        {
            "id": "user::transport_bins",
            "required_actors": [ "user::AMR" ]
        },
        {
            "id": "user::execute_order",
            "required_actors": [ "user::BinPickingStation" ]
        },
        {
            "id": "user::pack",
            "required_actors": [ "user::PackingStation" ]
        },
        {
            "id": "user::notify_done",
            "required_actors": [ "user::PackingStation" ]
        }
    ]
}
```

## Places Config

```json
{
    "places": [
        {
            "id": "order_entry",
            "type": "entrypoint",
            "params": {
                "new_actors": []
            }
        },
        {
            "id": "available_amrs",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::AMR",
                "initial_availability": 5
            }
        },
        {
            "id": "wait_for_amr",
            "type": "wait_with_timeout",
            "params": {
                "timeout_min": 10,
                "on_timeout": "error::no_amr_available"
            }
        },
        {
            "id": "transport_to_picking",
            "type": "action",
            "params": {
                "action_id": "user::transport_bins",
                "retries": 2,
                "timeout_per_try_min": 5,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "available_picking_stations",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::BinPickingStation",
                "initial_availability": 3
            }
        },
        {
            "id": "bin_picking",
            "type": "action",
            "params": {
                "action_id": "user::execute_order",
                "retries": 1,
                "timeout_per_try_min": 15,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "transport_to_packing",
            "type": "action",
            "params": {
                "action_id": "user::transport_bins",
                "retries": 2,
                "timeout_per_try_min": 5,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "available_packing_stations",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::PackingStation",
                "initial_availability": 2
            }
        },
        {
            "id": "packing",
            "type": "action",
            "params": {
                "action_id": "user::pack",
                "retries": 1,
                "timeout_per_try_min": 10,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "notify_completion",
            "type": "action",
            "params": {
                "action_id": "user::notify_done",
                "retries": 3,
                "timeout_per_try_s": 30,
                "failure_as_error": false,
                "error_to_global_handler": true
            }
        },
        {
            "id": "check_amr_charge",
            "type": "action",
            "params": {
                "action_id": "user::is_charged",
                "retries": 0,
                "timeout_per_try_s": 5,
                "failure_as_error": false,
                "error_to_global_handler": true
            }
        },
        {
            "id": "charge_amr",
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
            "id": "log_completion",
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
            "from": [ "order_entry" ],
            "to": [ "wait_for_amr" ]
        },
        {
            "from": [ "wait_for_amr", "available_amrs" ],
            "to": [ "transport_to_picking" ]
        },
        {
            "from": [ "transport_to_picking::success", "available_picking_stations" ],
            "to": [ "bin_picking" ]
        },
        {
            "from": [ "bin_picking::success" ],
            "to": [
                { "to": "transport_to_packing", "token_filter": "user::AMR" },
                { "to": "available_picking_stations", "token_filter": "user::BinPickingStation" }
            ]
        },
        {
            "from": [ "transport_to_packing::success", "available_packing_stations" ],
            "to": [ "packing" ]
        },
        {
            "from": [ "packing::success" ],
            "to": [
                { "to": "notify_completion" },
                { "to": "check_amr_charge", "token_filter": "user::AMR" },
                { "to": "available_packing_stations", "token_filter": "user::PackingStation" }
            ]
        },
        {
            "from": [ "notify_completion::success" ],
            "to": [ "log_completion" ]
        },
        {
            "from": [ "check_amr_charge::success" ],
            "to": [ "available_amrs" ]
        },
        {
            "from": [ "check_amr_charge::failure" ],
            "to": [ "charge_amr" ]
        },
        {
            "from": [ "charge_amr::success" ],
            "to": [ "available_amrs" ]
        }
    ]
}
```

## Error Handling

```json
{
    "global_error_handler": {
        "places": [
            {
                "id": "notify_operator",
                "type": "action",
                "params": {
                    "action_id": "system::notify_operator",
                    "retries": 3,
                    "timeout_per_try_s": 30
                }
            }
        ],
        "mapping": [
            {
                "error_id_filter": "*",
                "actor_filter": "user::AMR",
                "destination": "notify_operator"
            },
            {
                "error_id_filter": "error::no_amr_available",
                "destination": "notify_operator"
            }
        ]
    }
}
```

## Token Structures

AMR tokens:
```json
{
  "AMR": {
    "id": "amr_001",
    "Addr": "192.168.1.10:8080",
    "metadata": "zone_a"
  }
}
```

Order tokens (created at entrypoint):
```json
{
  "order_info": {
    "id": "order_12345",
    "content": ["item_a", "item_b"],
    "metadata": "priority_high"
  }
}
```
