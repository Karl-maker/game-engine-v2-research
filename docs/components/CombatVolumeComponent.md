# CombatVolumeComponent

## Purpose

- CombatVolumeComponent (descriptive only)
- Defines hit/hurt volumes for combat systems (damage, hit detection, etc).
- Typical usage:
- - Main character has multiple Hurt volumes (body, head)
- - Weapon (e.g., sword) has a Hit volume that is enabled during attack windows

## Use when

- Use `CombatVolumeComponent` when you need ECS data for combatvolumecomponent behavior.

## Enums

- `Role`: `Hurt`, `// receives damage
    Hit`, `// deals damage`
- `Shape`: `Sphere`, `Capsule`, `Box`

## Key fields

- `width` — Field of type `float` used by systems that consume this component.
- `height` — Field of type `float` used by systems that consume this component.
- `length` — Field of type `float` used by systems that consume this component.
- `radius` — Size, reach, or distance tuning.
- `role` — Field of type `Role` used by systems that consume this component.
- `shape` — Field of type `Shape` used by systems that consume this component.
- `offset` — Spatial placement or relative offset.
- `box` — Field of type `BoxSize` used by systems that consume this component.
- `capsule` — Field of type `CapsuleSize` used by systems that consume this component.
- `sphere` — Field of type `SphereSize` used by systems that consume this component.
- `damageMultiplier` — Field of type `float` used by systems that consume this component.
- `damage` — Field of type `float` used by systems that consume this component.
- `damageType` — Field of type `std::string` used by systems that consume this component.
- `force` — Movement or force tuning.
- `enabled` — Enable or disable this part of the component.
- `singleHit` — Field of type `bool` used by systems that consume this component.
- `tags` — Field of type `std::vector<std::string>` used by systems that consume this component.
- `volumes` — Field of type `std::vector<Volume>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::CombatVolumeComponent>(entity);
component = ecs::CombatVolumeComponent{};
```

## Notes

- Keep `CombatVolumeComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
