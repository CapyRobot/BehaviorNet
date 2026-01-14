# BehaviorNet

A Petri net-based behavior control framework for robotics. Config-driven workflows with support for custom actors and actions.

## Quick Start

```bash
# Build and run tests
bazel test //runtime:all

# Run robot picking example
bazel run //examples/robot_picking:robot_picking_example
```

## Structure

- `runtime/` - C++ runtime library
- `examples/` - Example workflows
- `gui/` - Visual editor (submodule)

## License

Apache-2.0
