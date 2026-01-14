# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BehaviorNet is a Petri net-based behavior control framework for robotics, similar to BehaviorTree but using Petri net semantics. The system is config-driven, supports runtime token injection, and provides both programmatic and GUI interfaces.

**Status**: GUI editor implemented, C++ runtime in design phase.

**Implementation**: C++ with Bazel (runtime), React/TypeScript (GUI)

## GUI Editor

```bash
cd gui
npm install      # Install dependencies
npm run dev      # Start development server (http://localhost:5173)
npm run build    # Production build to gui/dist/
```

**Features:**
- Drag-and-drop places and transitions onto canvas
- Connect places to transitions via arcs
- Configure place-specific parameters in the Inspector panel
- Define actors and actions in the registry modal
- Export/import JSON config files
- Save/load to browser storage

## Key Design Documents

- `design/PROJECT.md` - High-level project requirements and decisions
- `design/DESIGN.md` - Detailed system architecture and module specifications
- `design/BLOG_INFO.md` - Design info extracted from original blog posts
- `design/usecases/` - Example use cases (EV charger management, car wash)

## Core Concepts

- **Places**: Hold tokens representing system state; can be passive or mapped to actions
- **Transitions**: Fire when enabled; support priority and token filtering
- **Tokens**: Collections of actors (C++ objects); can be empty for flow control; moved (not copied) between places
- **Actors**: C++ classes implementing `ActorInterface`; represent domain entities (Vehicle, Charger, Robot)
- **Actions**: Methods on actors returning `ActionResult` (SUCCESS, FAILURE, IN_PROGRESS, ERROR)

## Architecture Modules

1. **Core Petri Net Engine** - Net structure, token distribution, transition firing
2. **Token and Actor System** - Actor/action registration, token creation/validation
3. **Special Place Types** - ActionPlace (with subplaces), EntrypointPlace, ResourcePoolPlace, etc.
4. **Configuration Parser** - YAML/JSON config parsing, plugin loading
5. **Net Validator** - Static validation, propagation analysis, deadlock detection
6. **Runtime Controller** - Execution loop, state recovery from logs
7. **Action Execution System** - Async execution, retries, timeouts
8. **GUI Framework** - Web-based visual editor and runtime observability
9. **Service Integration Layer** - Abstract external service calls (HTTP, future ROS)

## Usage Modes (planned)

```bash
# Config-only mode
$ bnet --config net_config.yaml

# Plugin mode with custom actors
$ bnet --config net_config.yaml --load_plugin my_plugin.so
```

## Actor Registration Pattern

```cpp
BNET_REGISTER_ACTOR(VehicleActor, "Vehicle");
BNET_REGISTER_ACTOR_ACTION(VehicleActor, moveToLocation, "move_to_location");
```

## Action Place Subplaces

Action places have implicit subplaces: `::in_execution`, `::success`, `::failure`, `::error`. Transitions reference these (e.g., `move_to_charger::success`).

## Design Decisions

- Tokens are moved, not copied, ensuring they exist in exactly one place
- Transitions have optional priority (default: 1, higher = preferred)
- Actions execute asynchronously with configurable retries and timeouts
- Static validation on initialization; liveness issues are warnings, actor type mismatches are errors
- Global error handler implemented as a subnet with error type filtering

## Development Guidelines

**Always verify your work:**
- Write tests for new functionality
- Run tests before considering work complete: `bazel test //runtime:all`
- Ensure CI passes (both runtime and GUI)

**Code quality:**
- Follow clang-tidy rules (run `bazel build //runtime:all --aspects=@bazel_clang_tidy//clang_tidy:clang_tidy.bzl%clang_tidy_aspect --output_groups=report`)
- Clean up after edits: remove unused headers, dead code, unused variables
- Use modern C++17 idioms

**C++ style:**
- Use `#pragma once` for header guards
- Prefer `std::string_view` for non-owning string parameters
- Use `[[nodiscard]]` for functions where ignoring return value is likely a bug
- Mark single-argument constructors `explicit`
- Use trailing return types for complex template functions
- Opening braces on next line for functions, if/else, class/struct:
  ```cpp
  class Foo
  {
  public:
      void bar()
      {
          if (condition)
          {
              // ...
          }
      }
  };
  ```

**Comments:**
- Keep comments minimal; don't repeat what the code says
- Use `/// @brief` for doc comments, with extra context above if needed
- Put `@param` and `@return` at the bottom of the doc block
- Use `///<` for trailing comments on the same line or after a declaration
- Example:
  ```cpp
  /// @brief Get an actor from the token.
  /// @tparam T The actor type to retrieve.
  /// @return Reference to the actor.
  /// @throws ActorNotFoundError if not present.
  template <typename T>
  [[nodiscard]] const T& getActor() const;

  struct ActionInfo {
      std::string id;           ///< e.g., "user::move_to_location"
      std::string actorTypeId;  ///< e.g., "user::Vehicle"
      bool requiresToken;       ///< true if action needs Token input
  };
  ```

**Testing:**
- Tests live in `runtime/test/`
- Use descriptive test function names: `testFeatureName()`
- Test both success and error paths
