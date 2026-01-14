# BehaviorNet Market Fit & Strategy

## Executive Summary

BehaviorNet targets a specific niche within the robotics behavior control space: **multi-robot and multi-resource coordination**. Rather than competing head-to-head with BehaviorTree.CPP for general robotics, BehaviorNet leverages Petri net semantics to excel where behavior trees struggle.

**Positioning**: "BehaviorNet for fleets, BehaviorTree for robots."

---

## Competitive Landscape

### BehaviorTree.CPP (The Incumbent)

| Metric | Value |
|--------|-------|
| GitHub Stars | ~3,800 |
| Forks | ~792 |
| Contributors | 132 |
| Latest Release | v4.8.4 (Jan 2026) |

**Strengths:**
- Integrated into Nav2 (most popular ROS2 navigation stack)
- Used by MoveIt Pro
- Groot visualization tool
- Extensive documentation and tutorials
- Strong corporate backing (PickNik)

**Weaknesses:**
- Concurrency is bolted-on, not native
- Resource pool modeling is awkward
- No formal verification
- Designed for single-robot behavior trees

### Existing Petri Net Robotics Frameworks

| Project | Stars | Status |
|---------|-------|--------|
| PetriNetPlans | 31 | Academic, ROS1 focus |
| PNLab | <10 | Editor only, minimal activity |
| PIPE | ~200 | General PN tool, not robotics-specific |

**Key Insight**: No Petri net framework has achieved mainstream robotics adoption. The space is open for a well-executed, ROS2-native solution.

---

## BehaviorNet's Competitive Advantages

### 1. Native Concurrency Modeling
Behavior trees simulate concurrency through parallel nodes. Petri nets model it naturally through token distribution across places. This matters when coordinating multiple entities that operate truly in parallel.

### 2. Resource Pool Patterns
The `resource_pool` place type elegantly handles scenarios like:
- AMRs waiting for available charging stations
- Orders waiting for available picking stations
- Jobs waiting for available machines

In BehaviorTree.CPP, this requires complex blackboard manipulation and custom decorators.

### 3. Formal Verification
Petri nets have mature mathematical foundations for:
- **Deadlock detection** - Critical for production systems
- **Reachability analysis** - Can the system reach error states?
- **Boundedness checking** - Will tokens accumulate unboundedly?

BehaviorTree.CPP offers no equivalent formal analysis tools.

### 4. Compact Representation
Research shows Petri nets can represent the same state space with exponentially smaller graphs than FSMs. For complex multi-entity workflows, this means more maintainable configurations.

### 5. Scalability Narrative
"Adding more robots is as trivial as adding tokens" - This is a powerful, accurate selling point for fleet management scenarios.

---

## Target Domains

BehaviorNet excels where multiple entities compete for shared resources and workflows involve token splitting/merging.

### Primary Targets

| Domain | Key Differentiator | Use Case |
|--------|-------------------|----------|
| **Warehouse Automation** | Multi-AMR + multi-station coordination | `usecases/autonomous_warehouse.md` |
| **Multi-AGV Traffic Coordination** | Zone access control, deadlock-free routing | `usecases/multi_agv_coordination.md` |
| **Factory Floor Orchestration** | Multi-workstation assembly, parallel lines | `usecases/factory_floor_orchestration.md` |
| **EV Charging Networks** | Vehicle-to-charger allocation | `usecases/electrical_vehicle_chargers.md` |

### Secondary Targets (Future)

| Domain | Why BehaviorNet Fits |
|--------|---------------------|
| **Airport Ground Operations** | Multi-vehicle coordination (tugs, belt loaders, fuel trucks) |
| **Hospital Logistics** | Robot delivery with elevator/door resource contention |
| **Port Container Handling** | Crane + AGV coordination with berth allocation |

---

## Why Petri Nets vs Behavior Trees

### When to Use BehaviorTree.CPP
- Single robot behavior
- Hierarchical task decomposition
- Reactive behaviors (sense-plan-act loops)
- Teams already familiar with BT patterns

### When to Use BehaviorNet
- Multi-robot/multi-resource coordination
- Shared resource allocation (charging stations, workstations)
- Workflows requiring formal verification
- Systems where deadlock is a real risk
- Fleet management at scale

### The Honest Trade-off

| Aspect | Behavior Trees | Petri Nets |
|--------|---------------|------------|
| Human readability | Higher (tree structure) | Lower (graph structure) |
| Concurrency | Simulated | Native |
| Formal verification | None | Mature tools |
| Learning curve | Gentler | Steeper |
| Community size | Large | Niche |
| ROS2 ecosystem | Excellent | Needs work |

---

## Essential Features for Success

### Must Have (MVP)

1. **Working C++ Runtime**
   - Core Petri net engine with token movement
   - Action place execution with subplaces
   - Transition firing with priority
   - Resource pool place type

2. **ROS2 Integration**
   - ROS2 wrapper (non-dependent core)
   - Action server/client integration
   - Topic-based token injection

3. **Compelling Demo**
   - Multi-robot warehouse scenario running end-to-end
   - Side-by-side comparison showing BT.CPP complexity vs BehaviorNet simplicity

4. **Documentation**
   - "BehaviorNet for BT.CPP users" guide
   - Concept mapping (blackboard -> tokens, parallel nodes -> token distribution)

### High Value Differentiators

1. **Deadlock Detection**
   - Static analysis at config load time
   - Warnings for potential deadlocks
   - Errors for guaranteed deadlocks

2. **Reachability Analysis**
   - "Can error state X be reached?"
   - "Is state Y always reachable?"

3. **Runtime Observability**
   - Token visualization
   - Transition firing animation
   - Place state highlighting

4. **State Recovery**
   - Execution logging
   - Crash recovery from logs
   - Manual intervention for partial recovery

### Nice to Have

1. **HTTP Actor Interface** - Config-only integrations without C++
2. **LLM-Friendly Configs** - Auto-generation potential
3. **Groot-like Visualization Tool** - Would accelerate adoption

---

## Development Priority

Based on competitive analysis and target domain needs:

| Priority | Feature | Rationale |
|----------|---------|-----------|
| P0 | Core C++ runtime | Nothing works without this |
| P0 | Basic validation | Catch config errors early |
| P1 | ROS2 wrapper | Required for adoption |
| P1 | Deadlock detection | Key differentiator |
| P1 | Multi-robot demo | Proves the value proposition |
| P2 | State recovery | Industrial reliability |
| P2 | Runtime GUI observability | Already have editor |
| P3 | HTTP actor | Expands use cases |
| P3 | Formal verification tools | Advanced users |

---

## Go-to-Market Strategy

### Phase 1: Prove the Concept
- Complete C++ runtime
- Warehouse demo working
- Blog post: "Why we built BehaviorNet"
- Announce on ROS Discourse

### Phase 2: Early Adopters
- ROS2 integration
- Documentation
- Solicit feedback from warehouse/logistics companies
- ROSCon talk submission

### Phase 3: Community Growth
- Respond to issues quickly
- Accept contributions
- Consider OSRA membership for governance
- Partner with a robotics company for case study

---

## Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| BT.CPP adds formal verification | Low | High | Move fast, establish niche |
| No community adoption | Medium | High | Focus on compelling demos and docs |
| Scope creep (trying to replace BT.CPP) | Medium | Medium | Stay focused on multi-entity niche |
| ROS ecosystem ignores us | Medium | High | ROS2 integration is P1, not optional |

---

## Success Metrics

### 6-Month Goals
- Working runtime with 2+ use cases
- ROS2 wrapper published
- 50+ GitHub stars
- 1+ external contributor

### 12-Month Goals
- 200+ GitHub stars
- Production use by 1+ company
- ROSCon presentation
- 5+ external contributors

---

## References

- [BehaviorTree.CPP](https://github.com/BehaviorTree/BehaviorTree.CPP)
- [PetriNetPlans](https://github.com/iocchi/PetriNetPlans)
- [Petri Net Plans Research](https://link.springer.com/article/10.1007/s10458-010-9146-1)
- [BT.CPP Adoption Blog](https://www.behaviortree.dev/blog/bt.cpp%20in%20robotics/)
- [Real-time Petri Net Coordination](https://www.frontiersin.org/journals/robotics-and-ai/articles/10.3389/frobt.2024.1363041/full)
- [OSRA Governance Model](https://osralliance.org/)
