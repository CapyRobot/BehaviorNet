# Blog Post Design Information

This document extracts design concepts from the original blog posts that inspired BehaviorNet:
- https://capybot.com/2023/01/14/pnet_intro/
- https://capybot.com/2023/02/20/pnet_sw/
- https://capybot.com/2023/03/03/pnet_example/

## Petri Net Fundamentals

### Core Components

- **Places**: White circles that hold tokens and represent system states or conditions
- **Transitions**: Black rectangles that represent events or actions
- **Tokens**: Small circles indicating the current marking (state distribution) across places
- **Directed Arcs**: Connections between places and transitions showing token flow

### Execution Semantics

Transitions fire when enabled—meaning all input places contain at least one token. Upon firing, a transition atomically consumes one token from each input place and produces one token to each output place, advancing the system state.

## Design Patterns

### Communication Protocol Pattern
Models asynchronous multi-agent interactions (sender/receiver with acknowledgment handshaking).

### Resource Sharing Pattern
Controls concurrent access to limited resources using tokens to represent availability constraints.

### Producer-Consumer Pattern
Uses intermediate places to decouple production from consumption rates.

## Scalability Approaches

Two modeling approaches for fleet-level systems:

1. **Individual branches per agent** - Direct but scales linearly with fleet size
2. **Token-as-entity model** - Agents flow through shared execution paths as tokens

The second approach improves scalability but requires extensions like Coloured Petri Nets to track per-agent identity.

## Petri Net Variants

- **Extended Petri Nets**: Add inhibitor arcs that disable transitions when specific places are marked
- **Coloured Petri Nets**: Allow tokens to carry data values, enabling identity tracking
- **Finite Capacity Nets**: Restrict maximum tokens per place

## Robotics Stack Integration

| Layer | Suitability |
|-------|-------------|
| **Planning Layer** | Best suited - multi-robot coordination and resource allocation |
| **Executive Layer** | Less common - Behavior Trees typically better for sequential/parallel task decomposition |
| **Behavioral Layer** | Rarely used - hardware abstraction layers handle this |
| **Observability** | Excellent - independent monitoring, deadlock detection |

## Analytical Advantages

Petri Nets enable formal analysis to:
- Detect undesired markings (deadlocks, forbidden states)
- Verify model liveness and safety properties
- Validate design correctness before implementation

---

## Software Architecture

### Centralized Behavior Controller Pattern

The system uses a centralized behavior controller that encapsulates global behavior knowledge rather than distributing decision-making across multiple entities. This improves modularity and scalability.

### Core Components

**1. Petri Net Model Element**
- Maintains places, transitions, and tokens representing system state
- Provides interface for querying markings and triggering transitions
- Delegates action execution to external actors

**2. Behavior Controller**
- Determines which transitions to fire
- Manages action execution tied to places
- Handles token lifecycle and state transitions

**3. Action System**
- Actions are associated with places and execute when tokens occupy them
- Asynchronous task execution with status returns: `{success, failure, in_progress}`
- Configuration-driven action definitions (HTTP requests, sleep operations, ROS services)
- Parameter injection from token data using special keywords like `@token{address}`

### Key Design Constraints

**Auto-Triggering (AT) Transitions**

Transitions with no action-associated input places should automatically trigger when enabled. This prevents unnecessary complexity in decision logic.

**Single Input Place Rule**

A non-AT transition shall have no more than a single input place, ensuring firing decisions can be made deterministically upon action completion.

### Token Data Structure

Tokens are implemented as dictionary-like structures (JSON/Python dicts) to maintain genericity:

```json
{"robot_id": "ABC", "ip": "192.168.1.1", ...}
```

**Token Transformation Rules:**
1. Output tokens concatenate input token data by default
2. Arc configuration parameters filter which top-level keys propagate through transitions

### Execution Loop

```
Epoch loop:
1. Check occupied places for new tokens
2. Execute associated actions asynchronously
3. On action completion (success/failure), trigger appropriate transitions
4. Retrigger in_progress actions in next cycle
```

---

## Example: Autonomous eCommerce Warehouse

### Use Case Overview

Models a fulfillment center with Autonomous Mobile Robots (AMRs) and robotic stations executing two parallel tasks.

**Order Execution Branch:**
1. AMR transports bins from warehouse to bin-picking station
2. Robotic arm picks requested items into output bin
3. AMR returns empty bins to warehouse
4. Second AMR delivers order to packing station
5. Packing station processes and notifies completion

**Replenishment Branch:**
1. AMR retrieves nearly-empty warehouse bin
2. Operator manually refills at replenishment center
3. AMR returns filled bin to warehouse

### Petri Net Structure

**Places represent:**
- Robot availability pools
- Station occupancy states
- Task progression checkpoints
- Charging stations

**Tokens carry entity data:**
```json
{
  "Robot_type": {"id": "...", "address": "...", "metadata": {}},
  "order_info": {"id": "...", "content": "...", "metadata": {}}
}
```

### Token Flow Design

Tokens transition through places by:
1. Acquiring necessary resources (robot, station)
2. Executing skills via HTTP requests
3. Releasing resources upon completion
4. Conditional routing based on action results

### Conditional Transitions

Example: Charging requirement - when an AMR completes a task, the transition checks battery level:
- **Success** → routes to "available AMRs" pool
- **Failure** → routes to charging station

This uses "acceptable action results" parameters on input arcs.

### Configuration-Driven Actions

Actions map to HTTP calls with token parameter substitution using patterns like `@token{data_key}` to dynamically inject robot addresses and order information.

---

## Strengths and Weaknesses

### Strengths
- Intuitive component-to-place mapping
- Clear concurrent process representation
- Formal mathematical semantics enabling automated verification
- Well-suited for distributed systems and multi-agent control

### Weaknesses
- Lack of inherent constraints can produce overly complex models
- Better suited for specific use cases rather than general behavioral control
