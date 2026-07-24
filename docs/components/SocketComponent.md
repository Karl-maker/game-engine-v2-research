# SocketComponent

## Purpose

- SocketComponent
- Lightweight bone/socket attachment descriptor. A socket entity can target a named entity and skeleton,
- then the socket system stores the computed world transform for attachment or targeting.

## Use when

- Use `SocketComponent` when you need ECS data for socketcomponent behavior.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `name` — A stable content or debug identifier.
- `targetEntity` — Reference to another entity or input source.
- `targetEntityName` — Reference to another entity or input source.
- `skeletonName` — Field of type `std::string` used by systems that consume this component.
- `boneName` — Field of type `std::string` used by systems that consume this component.
- `positionOffset` — Spatial placement or relative offset.
- `rotationOffset` — Spatial placement or relative offset.
- `scaleOffset` — Spatial placement or relative offset.
- `worldTransform` — Field of type `math::Mat4` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SocketComponent>(entity);
component = ecs::SocketComponent{};
```

## Notes

- Keep `SocketComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
