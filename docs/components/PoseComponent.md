# PoseComponent

## Purpose

- PoseComponent (data only)
- Describes weighted pose overrides applied to a SkeletonComponent by PoseSystem.

## Use when

- Use `PoseComponent` when you need ECS data for posecomponent behavior.

## Key fields

- `boneKey` — Field of type `std::string` used by systems that consume this component.
- `weight` — Field of type `float` used by systems that consume this component.
- `hasTranslation` — Field of type `bool` used by systems that consume this component.
- `translation` — Field of type `math::Vec3` used by systems that consume this component.
- `hasRotationEulerDeg` — Orientation or angle tuning.
- `rotationEulerDeg` — Orientation or angle tuning.
- `hasRotationQuat` — Orientation or angle tuning.
- `rotation` — Orientation or angle tuning.
- `hasScale` — Size, reach, or distance tuning.
- `scale` — Size, reach, or distance tuning.
- `name` — A stable content or debug identifier.
- `enabled` — Enable or disable this part of the component.
- `bones` — Field of type `std::vector<BoneOverride>` used by systems that consume this component.
- `poses` — Field of type `std::vector<Pose>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::PoseComponent>(entity);
component = ecs::PoseComponent{};
```

## Notes

- Keep `PoseComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
