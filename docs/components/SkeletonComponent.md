# SkeletonComponent

## Purpose

- SkeletonComponent stores gameplay or rendering data for ECS systems.

## Use when

- Use `SkeletonComponent` when you need ECS data for skeletoncomponent behavior.

## Enums

- `UpdateMode`: `Always`, `WhenVisible`, `Manual`
- `Space`: `Local`, `World`
- `Path`: `Translation`, `Rotation`, `Scale`

## Key fields

- `key` — Field of type `std::string` used by systems that consume this component.
- `parentIndex` — Field of type `int` used by systems that consume this component.
- `localBindTransform` — Field of type `math::Mat4` used by systems that consume this component.
- `boneIndex` — Field of type `int` used by systems that consume this component.
- `path` — Field of type `Path` used by systems that consume this component.
- `times` — Field of type `std::vector<float>` used by systems that consume this component.
- `vec3Values` — Numeric value used by a system or widget.
- `quatValues` — Numeric value used by a system or widget.
- `name` — A stable content or debug identifier.
- `durationSeconds` — Field of type `float` used by systems that consume this component.
- `channels` — Field of type `std::vector<Channel>` used by systems that consume this component.
- `enabled` — Enable or disable this part of the component.
- `skeletonId` — A stable content or debug identifier.
- `skeletonData` — Field of type `std::string` used by systems that consume this component.
- `rootBone` — Field of type `int` used by systems that consume this component.
- `bones` — Field of type `std::vector<Bone>` used by systems that consume this component.
- `boneCount` — Field of type `int` used by systems that consume this component.
- `currentPose` — Field of type `std::vector<math::Mat4>` used by systems that consume this component.
- `bindPose` — Field of type `std::vector<math::Mat4>` used by systems that consume this component.
- `inverseBindMatrices` — Field of type `std::vector<math::Mat4>` used by systems that consume this component.
- `animationClips` — Field of type `std::vector<AnimationClip>` used by systems that consume this component.
- `updateMode` — Field of type `UpdateMode` used by systems that consume this component.
- `space` — Field of type `Space` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SkeletonComponent>(entity);
component = ecs::SkeletonComponent{};
```

## Notes

- Keep `SkeletonComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
