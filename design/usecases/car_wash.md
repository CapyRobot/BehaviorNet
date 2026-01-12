# Electrical Vehicle Recharging Stations Management

In this use case, vehicles arrive at the car wash station network via an entrypoint. There is one single car wash station, and vehicles shall wait in a queue. The system models a place for vehicles waiting, currently washing, and a exit place for vehicles after washing.

Net:
```
(entrypoint / washing queue) -> (car_wash_station; capacity: 1) -> (exit)
```

## Actors

> NOTE: This is an example conceived before designing the controller. APIs and configs are only a representation of what we want to achieve at this point and may not be accurate.

The car washing station has an HTTP interface for controls and status querrying. So, we can use the default HTTP actor to interact with the car washing station, and do not need to define a custom actor.

## User Types Config

This info will be likely used to verify the net validity. I am not convinced that this is really needed. Maybe we can extract this directly from the codea bove.

```json
{
    "actors": [
        {
            "id": "user::CarWashStation",
            "type": "http_actor",
            "required_init_params": {
                "IpAddr": { "type": "str" },
            },
            "optional_init_params": {
            },
            "services": [
                {
                    "id": "request_to_wash_next_car",
                    "description": "Request to wash the next car. This includes requesting the car to enter the station, washing it and requesting the car to exit the station.",
                    "method": "POST",
                    "url": "/api/v1/car_wash/wash_next_car",
                    "body": {},
                    "response_path": "$.success" // used to extract request status from response
                },
                {
                    "id": "request_attendant_assistance",
                    "description": "Request assistance from a human attendant in case any errors.",
                    "method": "POST",
                    "url": "/api/v1/car_wash/request_attendant_assistance",
                    "body": {},
                    "response_path": "$.success" // used to extract request status from response
                }
            ]
        }
    ],
    "actions": [
        {
            "id": "user::request_to_wash_next_car",
            "required_actors": [ "user::CarWashStation" ],
            "type": "http_actor_service",
            "params": {
                "actor_id": "user::CarWashStation",
                "service_id": "request_to_wash_next_car",
                "service_params": {
                    "body": {}
                }
            }
        },
        {
            "id": "user::request_attendant_assistance",
            "required_actors": [ "user::CarWashStation" ],
            "type": "http_actor_service",
            "params": {
                "actor_id": "user::CarWashStation",
                "service_id": "request_attendant_assistance",
                "service_params": {
                    "body": {}
                }
            }
        }
    ]
}
```

## Places Config

```json
{
    "places": [
        {
            "id": "car_wash_entry_queue",
            "type": "entrypoint",
            "params": {
                "entry_mechanism": "TBD - HTTP? Manual? ROS topic?",
                "new_actors": [] // empty token, no car info available
            }
        },
        {
            "id": "wash_next_car",
            "type": "action",
            "params": {
                "action_id": "user::request_to_wash_next_car",
                "timeout_per_try_min": 10,
                "failure_as_error": true, // no failure place
                "error_to_global_handler": true // implicit transition from move_to_allocated_charger::error to global_error_handler
            },
            "token_capacity": 1,
            "token_requirements": [
                {
                    "requirement_type": "has_only",
                    "actor_types": [ "user::CarWashStation" ]
                }
            ]
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
            "from" : [ "car_wash_entry_queue" ],
            "to" : [ "request_to_wash_next_car" ]
        },
        {
            "from" : [ "wash_next_car::success" ],
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
            {
                "id": "request_attendant_assistance",
                "type": "action",
                "params": {
                    "action_id": "user::request_attendant_assistance",
                    "required_actors": [ "user::CarWashStation" ],
                    "type": "http_actor_service",
                    "params": {
                        "actor_id": "user::CarWashStation",
                        "service_id": "request_attendant_assistance",
                    }
                }
            }
        ],
        "mapping": [
            {
                "error_id_filter": "*", // errors of this type
                "actor_filter": "user::CarWashStation", // which contain a car wash station actor
                "destination": "request_attendant_assistance"
            }
        ]
    }
}
```

