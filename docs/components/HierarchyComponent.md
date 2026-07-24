# HierarchyComponent

## Purpose

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

## Use when

- Use `HierarchyComponent` when you need ECS data for hierarchycomponent behavior.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `parentEntity` — Field of type `EntityId` used by systems that consume this component.
- `children` — Field of type `std::vector<EntityId>` used by systems that consume this component.
- `localPosition` — Spatial placement or relative offset.
- `localRotation` — Orientation or angle tuning.
- `localScale` — Size, reach, or distance tuning.
- `inheritPosition` — Spatial placement or relative offset.
- `inheritRotation` — Orientation or angle tuning.
- `inheritScale` — Size, reach, or distance tuning.

## Example

```cpp
auto& component = registry.emplace<ecs::HierarchyComponent>(entity);
component = ecs::HierarchyComponent{};
```

## Notes

- Keep `HierarchyComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
