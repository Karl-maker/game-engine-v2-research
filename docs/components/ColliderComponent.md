# ColliderComponent

## What it is

- ColliderComponent (descriptive only)
- Describes collision shape and settings (not movement).
- A physics/collision system interprets this component.
- Terrain note:
- - Terrain colliders refer to a TerrainComponent entity id as their "source".
- - A terrain-height provider service (see `terrain::ITerrainHeightProvider`) can expose height sampling
- based on TerrainComponent + noise settings.

## When to use it

- Use this when an entity needs to collide, trigger, or participate in physics queries.

## How it fits

- Treat `ColliderComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Choose the shape that matches the gameplay need: box, sphere, capsule, mesh, or terrain.
- Use `isTrigger` for overlap-only interactions.
- Set `terrain.enabled` when the collider should follow a terrain height source instead of a primitive.

## Common recipes

- **Pickup trigger:** use a small box collider with `isTrigger = true`.
- **Character body:** use a capsule collider or a box collider sized to the actor.
- **Terrain collision:** enable the terrain source and point it at the terrain entity so physics can sample the heightfield.

## Enums

- `Shape`: `Box`, `Sphere`, `Capsule`, `Mesh`, `Terrain`
- `TerrainColliderType`: `Heightfield`

## Field guide

- `shape` — Enum or bitmask value that changes system behavior.
- `size` — Size, reach, or distance tuning.
- `offset` — Spatial placement or directional tuning.
- `isTrigger` — Field of type `bool` consumed by systems that read this component.
- `collisionLayer` — Enum or bitmask value that changes system behavior.
- `hasMesh` — Stable reference used by content, loaders, or rendering systems.
- `meshId` — Stable reference used by content, loaders, or rendering systems.
- `enabled` — Master on/off switch or similar behavior flag.
- `terrain.enabled` — Master on/off switch for the terrain collider source.
- `terrain.sourceTerrainEntity` — Reference to the terrain entity that owns the heightfield.
- `terrain.colliderType` — Enum or bitmask value that changes system behavior.
- `terrain.collisionLayer` — Enum or bitmask value that changes system behavior.
- `thicknessMeters` — Size, reach, or distance tuning.
- `terrain` — Field of type `TerrainSource` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::ColliderComponent>(entity);
component = ecs::ColliderComponent{};
```

## Common pairings

- Keep collision filtering in sync with your physics or gameplay layer rules.
- Use the mesh or terrain collider only when a primitive collider is too coarse.

## Notes

- Keep `ColliderComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
