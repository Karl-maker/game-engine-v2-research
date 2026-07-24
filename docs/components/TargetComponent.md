# TargetComponent

## What it is

- TargetComponent (orientation only)
- Describes how an entity should orient toward a target.
- Notes:
- - This component is descriptive only; a system applies the rotation.
- - It does not move the entity, it only affects orientation.

## When to use it

- Use this for aim assist, facing behavior, lock-on targeting, or chase logic.

## How it fits

- Treat `TargetComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Populate the target entity first, then let downstream systems align orientation or movement.

## Field guide

- `targetEntity` — Reference to another entity or a linked runtime object.
- `targetOffset` — Spatial placement or directional tuning.
- `upVector` — Field of type `math::Vec3` consumed by systems that read this component.
- `rotationSpeed` — Orientation or angular tuning.
- `lockRoll` — Orientation or angular tuning.
- `enabled` — Master on/off switch or similar behavior flag.

## Example

```cpp
auto& component = registry.emplace<ecs::TargetComponent>(entity);
component = ecs::TargetComponent{};
```

## Common pairings

- Pair `TargetComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `TargetComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
