# CombatVolumeComponent

## What it is

- CombatVolumeComponent (descriptive only)
- Defines hit/hurt volumes for combat systems (damage, hit detection, etc).
- Typical usage:
- - Main character has multiple Hurt volumes (body, head)
- - Weapon (e.g., sword) has a Hit volume that is enabled during attack windows

## When to use it

- Use this for melee attacks, damage zones, invulnerability zones, or other combat overlap logic.

## How it fits

- Treat `CombatVolumeComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Treat it as a transient gameplay shape and let a combat system decide how it applies damage.

## Enums

- `Role`: `Hurt`, `// receives damage
    Hit`, `// deals damage`
- `Shape`: `Sphere`, `Capsule`, `Box`

## Field guide

- `width` — Stable reference used by content, loaders, or rendering systems.
- `height` — Size, reach, or distance tuning.
- `length` — Size, reach, or distance tuning.
- `radius` — Size, reach, or distance tuning.
- `role` — Field of type `Role` consumed by systems that read this component.
- `shape` — Enum or bitmask value that changes system behavior.
- `offset` — Spatial placement or directional tuning.
- `box` — Field of type `BoxSize` consumed by systems that read this component.
- `capsule` — Field of type `CapsuleSize` consumed by systems that read this component.
- `sphere` — Field of type `SphereSize` consumed by systems that read this component.
- `damageMultiplier` — Numeric tuning used by gameplay or rendering systems.
- `damage` — Numeric tuning used by gameplay or rendering systems.
- `damageType` — Numeric tuning used by gameplay or rendering systems.
- `force` — Field of type `float` consumed by systems that read this component.
- `enabled` — Master on/off switch or similar behavior flag.
- `singleHit` — Field of type `bool` consumed by systems that read this component.
- `tags` — Field of type `std::vector<std::string>` consumed by systems that read this component.
- `volumes` — Field of type `std::vector<Volume>` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::CombatVolumeComponent>(entity);
component = ecs::CombatVolumeComponent{};
```

## Common pairings

- Pair `CombatVolumeComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `CombatVolumeComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
