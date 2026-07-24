# TransformComponent

## Purpose

- TransformComponent
- Any element can have a location/orientation/scale in the world.
- Attach it to an entity to give it spatial data.

## Use when

- Use this on almost every world object that needs a position, rotation, or scale.

## Key fields

- `position` — Spatial placement or relative offset.
- `rotation` — Orientation or angle tuning.
- `scale` — Size, reach, or distance tuning.

## Example

```cpp
auto& tr = registry.emplace<ecs::TransformComponent>(entity);
tr.position = {0.0f, 1.0f, 0.0f};
tr.rotation = {0.0f, 90.0f, 0.0f};
tr.scale = {1.0f, 1.0f, 1.0f};
```

## Notes

- Keep rotation in degrees so gameplay code matches the rest of the engine.
