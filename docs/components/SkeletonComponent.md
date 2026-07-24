# SkeletonComponent

## What it is

- SkeletonComponent stores data that systems and factories read to drive skeleton and bones.

## When to use it

- Use this for characters or props that need a bone hierarchy and skinning data.

## How it fits

- Treat `SkeletonComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Match the skeleton id and source asset to the same rig that the mesh loader reports.

## Enums

- `UpdateMode`: `Always`, `WhenVisible`, `Manual`
- `Space`: `Local`, `World`
- `Path`: `Translation`, `Rotation`, `Scale`

## Field guide

- `key` — Stable reference used by content, loaders, or rendering systems.
- `parentIndex` — Reference to another entity or a linked runtime object.
- `localBindTransform` — Field of type `math::Mat4` consumed by systems that read this component.
- `boneIndex` — Field of type `int` consumed by systems that read this component.
- `path` — Field of type `Path` consumed by systems that read this component.
- `times` — Field of type `std::vector<float>` consumed by systems that read this component.
- `vec3Values` — Numeric tuning used by gameplay or rendering systems.
- `quatValues` — Numeric tuning used by gameplay or rendering systems.
- `name` — Stable reference used by content, loaders, or rendering systems.
- `durationSeconds` — Field of type `float` consumed by systems that read this component.
- `channels` — Field of type `std::vector<Channel>` consumed by systems that read this component.
- `enabled` — Master on/off switch or similar behavior flag.
- `skeletonId` — Stable reference used by content, loaders, or rendering systems.
- `skeletonData` — Field of type `std::string` consumed by systems that read this component.
- `rootBone` — Field of type `int` consumed by systems that read this component.
- `bones` — Field of type `std::vector<Bone>` consumed by systems that read this component.
- `boneCount` — Count or budget control for work done by the system.
- `currentPose` — Field of type `std::vector<math::Mat4>` consumed by systems that read this component.
- `bindPose` — Field of type `std::vector<math::Mat4>` consumed by systems that read this component.
- `inverseBindMatrices` — Field of type `std::vector<math::Mat4>` consumed by systems that read this component.
- `animationClips` — Stable reference used by content, loaders, or rendering systems.
- `updateMode` — Enum or bitmask value that changes system behavior.
- `space` — Field of type `Space` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SkeletonComponent>(entity);
component = ecs::SkeletonComponent{};
```

## Common pairings

- Pair `SkeletonComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `SkeletonComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
