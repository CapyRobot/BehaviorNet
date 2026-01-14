# Factory Floor Orchestration

This use case models a manufacturing floor with multiple parallel assembly lines, shared workstations, and quality control checkpoints. Products flow through the system as tokens, competing for shared resources like specialized machines and technicians.

## Overview

The Petri net approach is ideal for factory orchestration because:
- **Parallel production lines**: Multiple products processed concurrently
- **Shared equipment**: Specialized machines serve multiple lines
- **Quality gates**: Products branch based on inspection results
- **Resource contention**: Technicians and tools as limited resources

### Why BehaviorNet vs BehaviorTree.CPP

In BehaviorTree.CPP, factory orchestration requires:
- One behavior tree per product (or complex parameterization)
- Blackboard-based resource locking with race conditions
- Manual coordination between independent trees
- Complex state machines for quality branching

In BehaviorNet:
- Products are tokens flowing through shared infrastructure
- Workstations are resource pools with natural contention handling
- Quality branches are simple transitions with conditions
- Parallel lines share the same net definition

## Scenario

An electronics manufacturing facility has:
- 2 parallel SMT (Surface Mount Technology) assembly lines
- 3 shared pick-and-place machines
- 2 reflow ovens
- 1 AOI (Automated Optical Inspection) station
- 2 manual rework stations
- 3 technicians for rework operations

Products must:
1. Go through SMT assembly (pick-and-place)
2. Pass through reflow oven
3. Be inspected by AOI
4. If failed: route to rework (requires technician)
5. If passed: proceed to final assembly
6. Complete final packaging

## Actors

```cpp
#include <bnet/ActorInterface.hpp>

class ProductActor : public bnet::ActorInterface
{
public:
    ProductActor(bnet::ActorInitDict initDict)
        : m_productId(initDict["product_id"])
        , m_batchId(initDict["batch_id"])
        , m_productType(initDict["product_type"])
    {
    }

    std::string getProductId() const { return m_productId; }
    std::string getBatchId() const { return m_batchId; }
    std::string getProductType() const { return m_productType; }

private:
    std::string m_productId;
    std::string m_batchId;
    std::string m_productType;
};

class WorkstationActor : public bnet::ActorInterface
{
public:
    WorkstationActor(bnet::ActorInitDict initDict)
        : m_stationId(initDict["id"])
        , m_stationType(initDict["station_type"])
        , m_stationAddr(initDict["Addr"])
    {
        assertValidConnection();
    }

    bnet::ActionResult processProduct(bnet::Token const& token)
    {
        auto productId = token.getActor<ProductActor>().getProductId();
        return sendProcessCommand(productId);
    }

    bnet::ActionResult inspect(bnet::Token const& token)
    {
        auto productId = token.getActor<ProductActor>().getProductId();
        auto result = runInspection(productId);
        if (result.passed)
            return bnet::ActionResult::SUCCESS;
        return bnet::ActionResult::FAILURE;
    }

    std::string getStationId() const { return m_stationId; }

private:
    void assertValidConnection();
    bnet::ActionResult sendProcessCommand(std::string const& productId);
    struct InspectionResult { bool passed; std::vector<std::string> defects; };
    InspectionResult runInspection(std::string const& productId);

    std::string m_stationId;
    std::string m_stationType;
    std::string m_stationAddr;
};

class TechnicianActor : public bnet::ActorInterface
{
public:
    TechnicianActor(bnet::ActorInitDict initDict)
        : m_techId(initDict["id"])
        , m_specialization(initDict["specialization"])
    {
    }

    bnet::ActionResult performRework(bnet::Token const& token)
    {
        auto productId = token.getActor<ProductActor>().getProductId();
        return executeRework(productId);
    }

    std::string getTechId() const { return m_techId; }

private:
    bnet::ActionResult executeRework(std::string const& productId);

    std::string m_techId;
    std::string m_specialization;
};

BNET_REGISTER_ACTOR(ProductActor, "Product");
BNET_REGISTER_ACTOR(WorkstationActor, "Workstation");
BNET_REGISTER_ACTOR(TechnicianActor, "Technician");

BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(WorkstationActor, processProduct, "process_product");
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(WorkstationActor, inspect, "inspect");
BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN_INPUT(TechnicianActor, performRework, "perform_rework");
```

## User Types Config

```json
{
    "actors": [
        {
            "id": "user::Product",
            "required_init_params": {
                "product_id": { "type": "str" },
                "batch_id": { "type": "str" },
                "product_type": { "type": "str" }
            },
            "optional_init_params": {
                "priority": { "type": "int" }
            }
        },
        {
            "id": "user::Workstation",
            "required_init_params": {
                "id": { "type": "str" },
                "station_type": { "type": "str" },
                "Addr": { "type": "str" }
            },
            "optional_init_params": {}
        },
        {
            "id": "user::Technician",
            "required_init_params": {
                "id": { "type": "str" },
                "specialization": { "type": "str" }
            },
            "optional_init_params": {}
        }
    ],
    "actions": [
        {
            "id": "user::process_product",
            "required_actors": [ "user::Workstation" ]
        },
        {
            "id": "user::inspect",
            "required_actors": [ "user::Workstation" ]
        },
        {
            "id": "user::perform_rework",
            "required_actors": [ "user::Technician" ]
        }
    ]
}
```

## Places Config

```json
{
    "places": [
        {
            "id": "product_entry",
            "type": "entrypoint",
            "params": {
                "new_actors": [ "user::Product" ]
            }
        },
        {
            "id": "smt_queue",
            "type": "passive",
            "params": {}
        },
        {
            "id": "available_pick_place_machines",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Workstation",
                "initial_availability": 3
            }
        },
        {
            "id": "smt_assembly",
            "type": "action",
            "params": {
                "action_id": "user::process_product",
                "retries": 1,
                "timeout_per_try_min": 10,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "reflow_queue",
            "type": "passive",
            "params": {}
        },
        {
            "id": "available_reflow_ovens",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Workstation",
                "initial_availability": 2
            }
        },
        {
            "id": "reflow_process",
            "type": "action",
            "params": {
                "action_id": "user::process_product",
                "retries": 0,
                "timeout_per_try_min": 15,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "aoi_queue",
            "type": "passive",
            "params": {}
        },
        {
            "id": "available_aoi_stations",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Workstation",
                "initial_availability": 1
            }
        },
        {
            "id": "aoi_inspection",
            "type": "action",
            "params": {
                "action_id": "user::inspect",
                "retries": 0,
                "timeout_per_try_min": 5,
                "failure_as_error": false,
                "error_to_global_handler": true
            }
        },
        {
            "id": "rework_queue",
            "type": "passive",
            "params": {}
        },
        {
            "id": "available_technicians",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Technician",
                "initial_availability": 3
            }
        },
        {
            "id": "available_rework_stations",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Workstation",
                "initial_availability": 2
            }
        },
        {
            "id": "rework_process",
            "type": "action",
            "params": {
                "action_id": "user::perform_rework",
                "retries": 2,
                "timeout_per_try_min": 20,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "final_assembly_queue",
            "type": "passive",
            "params": {}
        },
        {
            "id": "available_final_assembly_stations",
            "type": "resource_pool",
            "params": {
                "resource_id": "user::Workstation",
                "initial_availability": 2
            }
        },
        {
            "id": "final_assembly",
            "type": "action",
            "params": {
                "action_id": "user::process_product",
                "retries": 1,
                "timeout_per_try_min": 10,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "packaging",
            "type": "action",
            "params": {
                "action_id": "user::process_product",
                "retries": 1,
                "timeout_per_try_min": 5,
                "failure_as_error": true,
                "error_to_global_handler": true
            }
        },
        {
            "id": "log_completion",
            "type": "exit_logger"
        },
        {
            "id": "scrap_bin",
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
            "from": [ "product_entry" ],
            "to": [ "smt_queue" ]
        },
        {
            "from": [ "smt_queue", "available_pick_place_machines" ],
            "to": [ "smt_assembly" ]
        },
        {
            "from": [ "smt_assembly::success" ],
            "to": [
                { "to": "reflow_queue", "token_filter": "user::Product" },
                { "to": "available_pick_place_machines", "token_filter": "user::Workstation" }
            ]
        },
        {
            "from": [ "reflow_queue", "available_reflow_ovens" ],
            "to": [ "reflow_process" ]
        },
        {
            "from": [ "reflow_process::success" ],
            "to": [
                { "to": "aoi_queue", "token_filter": "user::Product" },
                { "to": "available_reflow_ovens", "token_filter": "user::Workstation" }
            ]
        },
        {
            "from": [ "aoi_queue", "available_aoi_stations" ],
            "to": [ "aoi_inspection" ]
        },
        {
            "from": [ "aoi_inspection::success" ],
            "to": [
                { "to": "final_assembly_queue", "token_filter": "user::Product" },
                { "to": "available_aoi_stations", "token_filter": "user::Workstation" }
            ]
        },
        {
            "from": [ "aoi_inspection::failure" ],
            "to": [
                { "to": "rework_queue", "token_filter": "user::Product" },
                { "to": "available_aoi_stations", "token_filter": "user::Workstation" }
            ]
        },
        {
            "from": [ "rework_queue", "available_technicians", "available_rework_stations" ],
            "to": [ "rework_process" ]
        },
        {
            "from": [ "rework_process::success" ],
            "to": [
                { "to": "aoi_queue", "token_filter": "user::Product" },
                { "to": "available_technicians", "token_filter": "user::Technician" },
                { "to": "available_rework_stations", "token_filter": "user::Workstation" }
            ]
        },
        {
            "from": [ "final_assembly_queue", "available_final_assembly_stations" ],
            "to": [ "final_assembly" ]
        },
        {
            "from": [ "final_assembly::success" ],
            "to": [
                { "to": "packaging" },
                { "to": "available_final_assembly_stations", "token_filter": "user::Workstation" }
            ]
        },
        {
            "from": [ "packaging::success" ],
            "to": [ "log_completion" ]
        }
    ]
}
```

## Quality Control Flow

The Petri net naturally models the quality branching:

```
                    ┌─────────────────────────────────────┐
                    │                                     │
                    v                                     │
[SMT] -> [Reflow] -> [AOI] --pass--> [Final Assembly] -> [Packaging] -> [Done]
                       │
                       └──fail──> [Rework] ─────────────────┘
                                     │
                                     └──max_retries_exceeded──> [Scrap]
```

Key insight: The `aoi_inspection::failure` transition routes products to rework, while `aoi_inspection::success` routes them to final assembly. This is a natural fit for Petri net semantics.

## Resource Contention Example

When multiple products compete for limited AOI stations:

1. Product A, B, C arrive at `aoi_queue`
2. Only 1 AOI station available
3. Product A acquires AOI (token moves from `available_aoi_stations`)
4. Products B, C wait in `aoi_queue`
5. Product A completes, AOI token returns to pool
6. Product B acquires AOI
7. etc.

This happens automatically without explicit locking code.

## Multi-Resource Acquisition

The rework process demonstrates acquiring multiple resources atomically:

```json
{
    "from": [ "rework_queue", "available_technicians", "available_rework_stations" ],
    "to": [ "rework_process" ]
}
```

The transition only fires when:
- A product is waiting in `rework_queue`
- A technician is available
- A rework station is available

All three resources are acquired atomically, preventing deadlocks.

## Error Handling

```json
{
    "global_error_handler": {
        "places": [
            {
                "id": "notify_supervisor",
                "type": "action",
                "params": {
                    "action_id": "system::notify_operator",
                    "retries": 3,
                    "timeout_per_try_s": 30
                }
            },
            {
                "id": "machine_recovery",
                "type": "action",
                "params": {
                    "action_id": "system::attempt_recovery",
                    "retries": 1,
                    "timeout_per_try_min": 5
                }
            },
            {
                "id": "scrap_product",
                "type": "action",
                "params": {
                    "action_id": "system::log_scrap",
                    "retries": 1,
                    "timeout_per_try_s": 10
                }
            }
        ],
        "mapping": [
            {
                "error_id_filter": "error::machine_failure",
                "actor_filter": "user::Workstation",
                "destination": "machine_recovery"
            },
            {
                "error_id_filter": "error::rework_max_attempts",
                "actor_filter": "user::Product",
                "destination": "scrap_product"
            },
            {
                "error_id_filter": "*",
                "destination": "notify_supervisor"
            }
        ]
    }
}
```

## Token Structures

Product tokens:
```json
{
  "Product": {
    "product_id": "PCB-2024-001234",
    "batch_id": "BATCH-2024-01-15-A",
    "product_type": "controller_board_v2",
    "priority": 1
  }
}
```

Workstation tokens:
```json
{
  "Workstation": {
    "id": "PNP-01",
    "station_type": "pick_and_place",
    "Addr": "192.168.1.50:8080"
  }
}
```

Technician tokens:
```json
{
  "Technician": {
    "id": "TECH-003",
    "specialization": "smt_rework"
  }
}
```

## Metrics and Observability

The Petri net structure naturally exposes metrics:
- **Queue lengths**: Token count in queue places
- **Resource utilization**: (Total - Available) / Total for each pool
- **Throughput**: Tokens passing through `log_completion` per time unit
- **Rework rate**: Tokens routed to rework vs total inspected
- **Cycle time**: Time from entry to completion per token

## Key Insights

1. **Shared infrastructure**: Multiple product types can flow through the same net
2. **Natural quality branching**: Inspection success/failure maps to transition firing
3. **Multi-resource coordination**: Rework needs technician + station atomically
4. **Bottleneck visibility**: Queue place token counts reveal bottlenecks
5. **Scalability**: Add machines = add tokens to resource pools
6. **Batch processing**: Tokens can carry batch IDs for traceability
