# HierarchyComponent

## What it is

- HierarchyComponent (descriptive ownership/parenting)
- Allows entities to own each other in a parent/child tree.
- Intended behavior (applied by a transform hierarchy system):
- - Child world transform is derived from parent world transform + child's local offset
- - Child keeps its own local offset (position/rotation/scale) relative to the parent
- Typical setup:
- - Parent entity has HierarchyComponent listing its children
- - Child entity has HierarchyComponent::parentEntity set to the parent
- Notes:
- - This component stores relationship + local offset only.
- - A dedicated system should compute/copy world transforms each frame.

## When to use it

- Use this when the entity should be logically grouped under a parent.

## How it fits

- Treat `HierarchyComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set the parent first, then let transform propagation resolve world-space placement.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `parentEntity` — Reference to another entity or a linked runtime object.
- `children` — Reference to another entity or a linked runtime object.
- `localPosition` — Spatial placement or directional tuning.
- `localRotation` — Orientation or angular tuning.
- `localScale` — Size, reach, or distance tuning.
- `inheritPosition` — Spatial placement or directional tuning.
- `inheritRotation` — Orientation or angular tuning.
- `inheritScale` — Size, reach, or distance tuning.

## Example

```cpp
auto& component = registry.emplace<ecs::HierarchyComponent>(entity);
component = ecs::HierarchyComponent{};
```

## Common pairings

- Pair `HierarchyComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `HierarchyComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
