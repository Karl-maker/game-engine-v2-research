# IKService

## What it is

- IKService
- Small gameplay-facing helper for configuring IKComponent chains without
- requiring callers to manually edit the component layout.

## When to use it

- Use this when animation or character setup needs to prepare IK chains before the solver runs.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
ecs::services::IKService ik;
// configure chains, targets, or helper state before the IK system runs.
```

## How it connects

- Integrate `IKService` where that responsibility belongs in the engine boundary.

## Notes

- Keep solver setup in one place so animation systems stay focused on pose evaluation.
