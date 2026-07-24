# TargetComponent

## Purpose

- TargetComponent (orientation only)
- Describes how an entity should orient toward a target.
- Notes:
- - This component is descriptive only; a system applies the rotation.
- - It does not move the entity, it only affects orientation.

## Use when

- Use `TargetComponent` when you need ECS data for targetcomponent behavior.

## Key fields

- `targetEntity` — Reference to another entity or input source.
- `targetOffset` — Spatial placement or relative offset.
- `upVector` — Field of type `math::Vec3` used by systems that consume this component.
- `rotationSpeed` — Orientation or angle tuning.
- `lockRoll` — Field of type `bool` used by systems that consume this component.
- `enabled` — Enable or disable this part of the component.

## Example

```cpp
auto& component = registry.emplace<ecs::TargetComponent>(entity);
component = ecs::TargetComponent{};
```

## Notes

- Keep `TargetComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
