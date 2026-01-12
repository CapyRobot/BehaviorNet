# BehaviorNet

Objective: implement a Petri Net behavior controller as described in the following blog posts:

Name: BehaviorNet

https://capybot.com/2023/01/14/pnet_intro/
https://capybot.com/2023/02/20/pnet_sw/
https://capybot.com/2023/03/03/pnet_example/

BehaviorNet is a Petri net-based behavior control framework for robotics, similar to BehaviorTree but using Petri net semantics. The system is config-driven, supports runtime token injection, and provides both programmatic and GUI interfaces for net definition, editing, and observability.

Ultimately this could be used standalone, but also as part of the ROS ecosystem. It should not depend on ROS, we will provide a wrapper later.

The net would be defined by a config file defining places, transitions, actions, etc. We will provide a GUI for editing/creating nets, as well for observability and controlling the system during actual runtime.

Implementation: C++ with Bazel build system

# Tokens

Tokens are a collection of actors (C++ objects). Actors are C++ classes implementing `ActorInterface` representing domain entities (e.g., Vehicle, Charger, Robot). An actor can be, e.g., a simple JSON-like data store or a robot. Just like in BT.CPP, the user will be able to define their actors and actions in C++ and that will be used by the framework. Tokens can be empty (e.g., used for control flow).

Actor types defined within the framework:

- JSON-like data store. This could be used, for example, for storing an execution order.
- HTTP based actor interface. Suppose a robot provides an HTTP interface, we could define this actor entirely on config files to interact with the robot.

Note that we want "service calls" to be abstracted since we want to implement many type of service implementation types (ROS, HTTP, etc). So, let's build the HTTP actor on top of that. From the configuration side, we should see no difference for service execution in an action for different impl types.

## Token Movement Model
- **Decision**: Tokens are **moved** (not copied or destroyed) when transitions fire
- **Rationale**: Ensures tokens exist in exactly one place, prevents duplication
- **Note**: Tokens can be empty (no actors) for flow control patterns

# Places

Hold tokens representing system state. Can have optional capacity limits.

A place can be passive (no actions) or mapped exactly to one action. Some actions are provided by the framework so users can use directly from the config files without the need to defining anything in C++. The user may specify token requirements for needed actors.

An action execution binds to a Token. It can read data from the Token and modify data in the token. The token is stuck in the place (not available for transitions) while its assossiated action is under execution.

Actions are methods on actors that can be invoked by action places. Actions return `ActionResult` (SUCCESS, FAILURE, IN_PROGRESS, or ERROR with error type). For conveniency, action places will have 4 subplaces: in execution, failure, success, error. Transition between the subplaces is automatic on action completion. The user should be able to configure number of attempts to retry on failure - the tokken is only moved to the failure place after all attempts fail. The user should also be able to configure "failure as error" - the failure subplace is not needed. On failure, tokens are moved to the error subplace.

There is a special place type for entrypoint, which allows the user to enter new tokens during runtime, e.g., new available resources, new orders to be executed. Entities allowed in an entrypoint shall be defined in config. Validates actor types match allowed_actors.

Basic actions provided by the framework:

- Delay. Returns `in progress` during user-provided delay period, then `success` on completion of that time
- Wait with Timeout. Waits for condition with timeout, moves to error on timeout.
- Boolean Condition Cheker. Checks a boolean condition either from the JSON-like data store or the HTTP actor interface.
- Execute Service. Service call (e.g., HTTP actor or user provided actor).
- Exit Logger Place: Logs token and destroys it (end of workflow).
- **Global Error Place**: Subnet for error handling with error type filtering. See the error handling section.

## Async Action Execution
- **Decision**: All actions execute asynchronously
- **Rationale**: Actions typically call external services, need non-blocking execution
- **Implementation**: Start action, periodically check status, move to subplace on completion

## Action Retries and Timeouts
- **Decision**: Actions support retry count and timeout per try
- **Rationale**: Handles transient failures gracefully
- **Configuration**:
  - `retries`: Number of retries on failure
  - `timeout_per_try_s`: Timeout per attempt
  - `timeout_as_error`: Treat timeout as error (true) or failure (false)
  - `failure_as_error`: Treat retry-exhausted failure as error

# Transitions

Transition arcs should be able to define requirements for certain actors (explicitely by the user in config, or automatically on analysis using entrypoints and explictely defined transitions).

## Transition Priority
- **Decision**: Transitions have optional integer priority (default: 1, higher = preferred)
- **Rationale**: Allows deterministic firing order when multiple transitions compete
- **Tie-breaking**: If priority tie, transition fired last in previous epoch has lower preference
- **Implementation**: Sort transitions by priority, fire enabled ones in order

## Tokens Priority
- **Decision**: Tokens that have been waiting in the same place while ready for longer have highier priority for being selected when firing transitions.
- **Rationale**: Avoids a token from being stuck for too long while other tokens how arrived later move on.
- **Implementation**: Tokens available to transitions can be stored in a fifo queue.

## Entity Transformation
- **Decision**: Tokens can be split/merged during transitions, actions can modify tokens
- **Rationale**: Supports complex workflows (e.g., split vehicle/charger after charging)
- **Implementation**: Token filtering in transitions, transformation API in actions

# Analisys

On startup, the controller should check for net correctness using propagation. E.g., an entrypoint for entity A connected to an outgoing arc requiring entity B is not allowed.

The net validator component should be part of the main app runtime so it can be executed on startup to detect issues, but it should also be used standalone to validate configs and run more comprehensive checks on-demand without the need to start the full runtime.

Need to ensure cycles don't cause unbounded token accumulation.

Deadlock Detection is essential for net analysis.

## Net Validation
- **Decision**: Static validation on initialization, liveness as warnings
- **Rationale**: Catch errors early, but allow potentially valid nets with warnings
- **Errors**: Actor type mismatches block runtime
- **Warnings**: Liveness issues, potential deadlocks

# Error Handling

There is a Global error place, which can be referenced by any actions which do not do special error handling locally. This is to facilitate and avoid too many error-related transitions.

C++ exception-like error types, filter by error type and actor type.

1. Action returns ERROR(error_type)
2. Token moves to action_place::error
3. If error_to_global_handler (optional action place parameter): token moves to global_error_handler
4. Global error handler (subnet):
   - Entrypoint receives token with error entity
   - Transitions filter by error type and actor type
   - Route to appropriate error handling place
   - Error handling actions could, for example:
     - Attempt automatic recovery
     - Request manual intervention
     - Move "fixed" tokens back to main net

# GUI

Responsibilities:
- Visual net editor (drag-and-drop)
- Runtime observability (token visualization, place states)
- Runtime control (token injection, pause/resume)
- Net validation feedback

Config should also store GUI metadata like placements for rendering.

Some Features:
- Visual place/transition editing
- Token flow animation
- Place state highlighting
- Transition firing indicators
- Entrypoint token injection interface
- Error place monitoring

WebGUI over an HTTP server.

# Robustness to crashing - state recovery

1. On startup, check for execution log
2. If log exists, attempt to recover state.
3. If full recovery fails, but partial recovery succeeds:
   - Move problematic tokens to error state
   - Request user manual intervention
4. Resume execution from recovered state

# App Execution Model

# Net Configuration

Should be LLM friendly. Future work include auto-generation and documentation autogen from config.

# Open Questions / Future Work

1. **Net Composition:**
   - Can nets be composed/hierarchical?
   - Should we support sub-nets as places?
   - How to handle net reuse/modularity?

   **Status**: Open for future consideration

