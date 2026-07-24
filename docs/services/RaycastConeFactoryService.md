# RaycastConeFactoryService

## Purpose

- RaycastConeFactoryService provides reusable engine behavior.

## Use when

- Builds cone-shaped raycast or sensor setups for visibility, targeting, or interaction checks.

## Example

```cpp
ecs::services::RaycastConeFactoryService service;
ecs::services::RaycastConeConfig cfg;
cfg.rayCount = 16;
cfg.coneAngleDeg = 25.0f;
auto rays = service.createCone(registry, owner, cfg);
```

## Notes

- This is especially useful for gameplay sensors that need a field-of-view style query.
