# ControlService

## What it is

- ControlService (demo-focused)
- Converts input (currently line commands from TickContext) into a simple control state
- that ControllerSystem can apply to ControllerComponents.

## When to use it

- Use this to bridge raw commands or realtime input into the rest of the ECS game loop.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
core::ControlService controls;
controls.update(ctx);
auto state = controls.state();
```

## How it connects

- Integrate `ControlService` where that responsibility belongs in the engine boundary.

## Notes

- This keeps input parsing out of movement and combat systems.
