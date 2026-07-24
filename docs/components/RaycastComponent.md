# RaycastComponent

## Purpose

- RaycastComponent (descriptive only)
- Describes ray/sphere-cast queries and stores hit results for systems to fill in.
- This component does not do any physics queries by itself.

## Use when

- Use `RaycastComponent` when you need ECS data for raycastcomponent behavior.

## Enums

- `OriginMode`: `Entity`, `// origin comes from `originEntity` (plus local offset)
    WorldPosition // origin uses `worldPosition` directly`
- `DirectionMode`: `Forward`, `Up`, `Down`, `Right`, `Left`, `CustomVector`, `TowardTarget`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `sensorEntity` — Field of type `EntityId` used by systems that consume this component.
- `raycastCategory` — Field of type `std::string` used by systems that consume this component.
- `raycastLayer` — Layer or filtering control.
- `originMode` — Field of type `OriginMode` used by systems that consume this component.
- `originEntity` — Field of type `EntityId` used by systems that consume this component.
- `localOffset` — Spatial placement or relative offset.
- `hasWorldPosition` — Spatial placement or relative offset.
- `worldPosition` — Spatial placement or relative offset.
- `directionMode` — Field of type `DirectionMode` used by systems that consume this component.
- `customDirection` — Field of type `math::Vec3` used by systems that consume this component.
- `towardTargetEntity` — Reference to another entity or input source.
- `towardTargetOffset` — Spatial placement or relative offset.
- `length` — Field of type `float` used by systems that consume this component.
- `radius` — Size, reach, or distance tuning.
- `collisionLayers` — Layer or filtering control.
- `ignoreLayers` — Layer or filtering control.
- `ignoreTriggerColliders` — Field of type `bool` used by systems that consume this component.
- `ignoreSelf` — Field of type `bool` used by systems that consume this component.
- `maxHits` — Field of type `int` used by systems that consume this component.
- `continuous` — Field of type `bool` used by systems that consume this component.
- `debugDraw` — Field of type `bool` used by systems that consume this component.
- `debugColor` — Color or tint control.
- `debugDurationSeconds` — Field of type `float` used by systems that consume this component.
- `hitResults` — Field of type `std::vector<physics::RaycastHit>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::RaycastComponent>(entity);
component = ecs::RaycastComponent{};
```

## Notes

- Keep `RaycastComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
