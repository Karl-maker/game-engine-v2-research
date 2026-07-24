# SocketComponent

## What it is

- SocketComponent
- Lightweight bone/socket attachment descriptor. A socket entity can target a named entity and skeleton,
- then the socket system stores the computed world transform for attachment or targeting.

## When to use it

- Use this when a model needs to expose a hand, head, or weapon socket.

## How it fits

- Treat `SocketComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Author socket names to match the attachments that your factories or gameplay code expect.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `name` — Stable reference used by content, loaders, or rendering systems.
- `targetEntity` — Reference to another entity or a linked runtime object.
- `targetEntityName` — Stable reference used by content, loaders, or rendering systems.
- `skeletonName` — Stable reference used by content, loaders, or rendering systems.
- `boneName` — Stable reference used by content, loaders, or rendering systems.
- `positionOffset` — Spatial placement or directional tuning.
- `rotationOffset` — Spatial placement or directional tuning.
- `scaleOffset` — Spatial placement or directional tuning.
- `worldTransform` — Field of type `math::Mat4` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SocketComponent>(entity);
component = ecs::SocketComponent{};
```

## Common pairings

- Pair `SocketComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `SocketComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
