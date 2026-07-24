# RaycastConeFactoryService

## What it is

- RaycastConeFactoryService (descriptive only)
- Creates a cone-shaped set of raycast entities:
- - Each raycast is its own entity with a RaycastComponent
- - Each raycast entity is attached to the owner via AttachmentComponent (inherits rotation)
- A raycast system can later use:
- - the raycast entity's attachment-derived transform (origin/orientation)
- - RaycastComponent parameters (length, radius, layers, etc)

## When to use it

- Use this when a gameplay system needs a spread of rays instead of a single line cast.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
ecs::services::RaycastConeFactoryService service;
ecs::services::RaycastConeConfig cfg;
cfg.rayCount = 16;
cfg.coneAngleDeg = 25.0f;
auto rays = service.createCone(registry, owner, cfg);
```

## How it connects

- Integrate `RaycastConeFactoryService` where that responsibility belongs in the engine boundary.

## Notes

- This is useful for AI vision, combat scans, and sensor cones.
