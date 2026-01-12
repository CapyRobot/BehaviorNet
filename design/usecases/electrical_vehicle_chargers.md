# Electrical Vehicle Recharging Stations Management

In this use case, electric vehicles (EVs) arrive at the recharging station network via an entrypoint. Each vehicle seeks an available charging station. The system models a place for tracking available charging stations and a separate place for vehicles currently charging. When a vehicle finishes charging, a transition moves it to the exit place (representing departure), and the charging station is returned to the pool of available chargers, ready to serve the next arriving vehicle.

## Actors

> NOTE: This is an example conceived before designing the controller. APIs and configs are only a representation of what we want to achieve at this point and may not be accurate.

```cpp

#include <bnet/ActorInterface.hpp>

class ChargerActor : public bnet::ActorInterface
{
public:
    ChargerActor(bnet::ActorInitDict initDict)
        : m_chargerIpAddr(initDict["IpAddr"])
        , m_chargerUniqueId(initDict["uniqueId"])
    {
        assertValidConnection(); // throw on failure to connect to charger
    }

    bnet::ActionResult checkChargingCompleted()
    {
        try
        {
            switch (getChargingStatus())
            {
                case ChargingStatus::DONE:
                    return bnet::ActionResult::SUCCESS;
                case ChargingStatus::IN_PROGRESS:
                    return bnet::ActionResult::IN_PROGRESS;
                case ChargingStatus::FAILURE:
                    return bnet::ActionResult::FAILURE;
            }
            return bnet::ActionResult::ERROR::UnreachableCodeBranch();
        }
        except(ConnectionLostException const& e)
        {
            return bnet::ActionResult::ERROR::ConnectionLostException(e);
        }
    }

    int getChargingBayNumber() const;

private:
    enum class ChargingStatus
    {
        DONE,
        IN_PROGRESS,
        FAILURE // driver should try connecting charger again
    };

    ChargingStatus getChargingStatus();
    void assertValidConnection();

    std::string m_chargerIpAddr;
    int m_chargerUniqueId;
};

class VehicleActor : public bnet::ActorInterface
{
public:
    VehicleActor(bnet::ActorInitDict initDict)
        : m_vehicleIpAddr(initDict["IpAddr"])
        , m_vehicleUniqueId(initDict["uniqueId"])
    {
        assertValidConnection(); // throw on failure to connect to charger
    }

    bnet::ActionResult moveToChargingBay(bnet::Token const& token)
    {
        int bay = token.getActor<ChargerActor>().getChargingBayNumber();
        requestUserToMoveToChargingBay(bay);
        if (waitForUserConfirmation())
        {
            return bnet::ActionResult::SUCCESS;
        }
        return bnet::ActionResult::FAILURE;
    }

    bnet::ActionResult requestExit()
    {
        requestUserToExit();
        if (waitForUserConfirmation())
        {
            return bnet::ActionResult::SUCCESS;
        }
        return bnet::ActionResult::FAILURE;
    }

private:
    void assertValidConnection();

    std::string m_vehicleIpAddr;
    int m_vehicleUniqueId;
};

BNET_REGISTER_ACTOR(ChargerActor, "Charger");
BNET_REGISTER_ACTOR(VehicleActor, "Vehicle");

BNET_REGISTER_ACTOR_ACTION(ChargerActor, checkChargingCompleted, "check_charger_status");
/*
Smething like:
class ChargingStatusAction : public ActionBase<ChargerActor>
{
    bnet::ActionResult operator()(bnet::Token const& token)
    {
        return token.getActor<ChargerActor>().checkChargingCompleted();
    }
}
*/

BNET_REGISTER_ACTOR_ACTION(VehicleActor, requestExit, "request_exit");
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(VehicleActor, moveToChargingBay, "request_move_to_charging_bay");
```

## User Types Config

This info will be likely used to verify the net validity. I am not convinced that this is really needed. Maybe we can extract this directly from the codea bove.

```json
{
    "actors": [
        {
           "id": "user::Vehicle",
            "required_init_params": {
                "IpAddr": { "type": "str" },
                "IpAddr": { "uniqueId": "int" }
            },
            "optional_init_params": {
            }
        }
        {
           "id": "user::Charger",
            "required_init_params": {
                "IpAddr": { "type": "str" },
                "IpAddr": { "uniqueId": "int" }
            },
            "optional_init_params": {
            }
        }
    ],
    "actions": [
        {
            "id": "user::request_exit",
            "required_actors": [ "user::Vehicle" ]
        },
        {
            "id": "user::request_move_to_charging_bay",
            "required_actors": [ "user::Vehicle", "user::Charger" ]
        },
        {
            "id": "user::check_charger_status",
            "required_actors": [ "user::Charger" ]
        }
    ]
}
```

## Places Config

```json
{
    "places": [
        {
            "id": "vehicle_entry",
            "type": "entrypoint",
            "params": {
                "entry_mechanism": "TBD - HTTP? Manual? ROS topic?",
                "new_actors": [
                    "id": "user::Vehicle"
                ]
            }
        },
        {
            "id": "wait_for_charger_allocation",
            "type": "wait_with_timeout", // cap wait time. Move to error on timeout
            "params": {
                "timeout_min": 5,
                "on_timeout": "error::request_manual_allocation"
            },
            "required_actors": [ "user::Vehicle" ] // optional, used to assert net validity
        },
        {
            "id": "available_chargers",
            "type": "resource_pool", // special type associate with user actions like update resources available, pull resource out for maintanance, etc
            "params": {
                "resource_id": "user::Charger"
            }
        },
        {
            "id": "move_to_allocated_charger",
            "type": "action",
            "params": {
                "action_id": "user::request_move_to_charging_bay",
                "action_params": { // TODO: actions should receive this data as input
                },
                "retries": 3,
                "timeout_per_try_s": 60,
                "failure_as_error": true, // no failure place
                "error_to_global_handler": true // implicit transition from move_to_allocated_charger::error to global_error_handler
            }
        },
        {
            "id": "charging",
            "type": "action",
            "params": {
                "action_id": "user::check_charger_status",
                "action_params": {
                },
                "retries": 1,
                "timeout_per_try_min": 60,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "exit",
            "type": "action",
            "params": {
                "action_id": "user::request_exit",
                "action_params": {
                },
                "retries": 3,
                "timeout_per_try_s": 60,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "log_completion",
            "type": "exit_logger" // log received token into and destroy it
        },
        // { - implictly created
        //     "id": "global_error_handler",
        //     "type": "global_error_handler"
        // },
    ]
}
```

## Transitions Config

```json
{
    "transitions": [
        {
            "from" : [ "vehicle_entry" ],
            "to" : [ "wait_for_charger_allocation" ]
        },
        {
            "from" : [ "wait_for_charger_allocation", "available_chargers" ],
            "to" : [ "move_to_allocated_charger" ]
        },
        {
            "from" : [ "move_to_allocated_charger::success" ],
            "to" : [ "charging" ]
        },
        {
            "from" : [ "charging::success" ],
            "to" : [
                { "to": "exit", "token_filter": "user::Vehicle" },
                { "to": "available_chargers", "token_filter": "user::Charger" },
            ]
        },
        {
            "from" : [ "exit::success" ],
            "to" : [ "log_completion" ]
        },
    ]
}
```

## Error Handling

```json
{
    "global_error_handler": {
        "places": [
            // Like a subnet where you can add places and actions to handle errors.
            // For example, we could have a place which try to reboot the machine and reach a good state.
            // We could have a place to notify a human operator to come take a look.
            // No transitions within the error handler, but we could have transitions to move "fixed" tokens into the net.
        ],
        "mapping": [
            {
                "error_id_filter": "connection_lost_error", // errors of this type
                "actor_filter": "user::Charger", // which contain a charger actor
                "destination": "..."
            }
        ]
    }
}
```

