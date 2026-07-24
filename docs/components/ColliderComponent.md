# ColliderComponent

## Purpose

- ColliderComponent (descriptive only)
- Describes collision shape and settings (not movement).
- A physics/collision system interprets this component.
- Terrain note:
- - Terrain colliders refer to a TerrainComponent entity id as their "source".
- - A terrain-height provider service (see `terrain::ITerrainHeightProvider`) can expose height sampling
- based on TerrainComponent + noise settings.

## Use when

- Use this when an entity needs a collision shape, trigger volume, mesh collider, or terrain collider.

## Enums

- `Shape`: `Box`, `Sphere`, `Capsule`, `Mesh`, `Terrain`
- `TerrainColliderType`: `Heightfield`

## Key fields

- `shape` — Field of type `Shape` used by systems that consume this component.
- `size` — Size, reach, or distance tuning.
- `offset` — Spatial placement or relative offset.
- `isTrigger` — Field of type `bool` used by systems that consume this component.
- `collisionLayer` — Layer or filtering control.
- `hasMesh` — Field of type `bool` used by systems that consume this component.
- `meshId` — A stable content or debug identifier.
- `enabled` — Enable or disable this part of the component.
- `sourceTerrainEntity` — Reference to another entity or input source.
- `colliderType` — Field of type `TerrainColliderType` used by systems that consume this component.
- `thicknessMeters` — Field of type `float` used by systems that consume this component.
- `terrain` — Field of type `}` used by systems that consume this component.

## Example

```cpp
auto& collider = registry.emplace<ecs::ColliderComponent>(entity);
collider.shape = ecs::ColliderComponent::Shape::Box;
collider.size = {1.0f, 2.0f, 1.0f};
collider.collisionLayer = physics::kAllLayers;
```

## Notes

- Keep collider data separate from movement so physics, raycasts, and triggers can reuse it.
- Use `terrain.enabled` when the collider should sample a terrain height provider instead of a simple primitive.
