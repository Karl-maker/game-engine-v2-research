# RaycastComponent

## What it is

- RaycastComponent (descriptive only)
- Describes ray/sphere-cast queries and stores hit results for systems to fill in.
- This component does not do any physics queries by itself.

## When to use it

- Use this for line-of-sight, ground checks, interaction checks, or debug rays.

## How it fits

- Treat `RaycastComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set length, layers, and max hit count first, then decide whether the query should ignore self or triggers.

## Enums

- `OriginMode`: `Entity`, `// origin comes from `originEntity` (plus local offset)
    WorldPosition // origin uses `worldPosition` directly`
- `DirectionMode`: `Forward`, `Up`, `Down`, `Right`, `Left`, `CustomVector`, `TowardTarget`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `sensorEntity` — Field of type `EntityId` consumed by systems that read this component.
- `raycastCategory` — Field of type `std::string` consumed by systems that read this component.
- `raycastLayer` — Enum or bitmask value that changes system behavior.
- `originMode` — Enum or bitmask value that changes system behavior.
- `originEntity` — Field of type `EntityId` consumed by systems that read this component.
- `localOffset` — Spatial placement or directional tuning.
- `hasWorldPosition` — Spatial placement or directional tuning.
- `worldPosition` — Spatial placement or directional tuning.
- `directionMode` — Spatial placement or directional tuning.
- `customDirection` — Spatial placement or directional tuning.
- `towardTargetEntity` — Reference to another entity or a linked runtime object.
- `towardTargetOffset` — Spatial placement or directional tuning.
- `length` — Size, reach, or distance tuning.
- `radius` — Size, reach, or distance tuning.
- `collisionLayers` — Enum or bitmask value that changes system behavior.
- `ignoreLayers` — Enum or bitmask value that changes system behavior.
- `ignoreTriggerColliders` — Stable reference used by content, loaders, or rendering systems.
- `ignoreSelf` — Field of type `bool` consumed by systems that read this component.
- `maxHits` — Field of type `int` consumed by systems that read this component.
- `continuous` — Field of type `bool` consumed by systems that read this component.
- `debugDraw` — Field of type `bool` consumed by systems that read this component.
- `debugColor` — Color, tint, or display styling.
- `debugDurationSeconds` — Field of type `float` consumed by systems that read this component.
- `hitResults` — Field of type `std::vector<physics::RaycastHit>` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::RaycastComponent>(entity);
component = ecs::RaycastComponent{};
```

## Common pairings

- Pair `RaycastComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `RaycastComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
