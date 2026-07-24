# PoseComponent

## What it is

- PoseComponent (data only)
- Describes weighted pose overrides applied to a SkeletonComponent by PoseSystem.

## When to use it

- Use this when animation or IK systems need a live bone pose to update every frame.

## How it fits

- Treat `PoseComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Keep pose output separate from authored animation input so both can be blended cleanly.

## Field guide

- `boneKey` — Stable reference used by content, loaders, or rendering systems.
- `weight` — Numeric tuning used by gameplay or rendering systems.
- `hasTranslation` — Field of type `bool` consumed by systems that read this component.
- `translation` — Field of type `math::Vec3` consumed by systems that read this component.
- `hasRotationEulerDeg` — Orientation or angular tuning.
- `rotationEulerDeg` — Orientation or angular tuning.
- `hasRotationQuat` — Orientation or angular tuning.
- `rotation` — Orientation or angular tuning.
- `hasScale` — Size, reach, or distance tuning.
- `scale` — Size, reach, or distance tuning.
- `name` — Stable reference used by content, loaders, or rendering systems.
- `enabled` — Master on/off switch or similar behavior flag.
- `bones` — Field of type `std::vector<BoneOverride>` consumed by systems that read this component.
- `poses` — Field of type `std::vector<Pose>` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::PoseComponent>(entity);
component = ecs::PoseComponent{};
```

## Common pairings

- Pair `PoseComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `PoseComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
