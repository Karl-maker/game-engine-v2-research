# ThirdPersonCameraComponent

## Purpose

- ThirdPersonCameraComponent stores gameplay or rendering data for ECS systems.

## Use when

- Use this when the camera should follow an entity with an offset and spring-like distance rules.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `target` — Reference to another entity or input source.
- `targetOffset` — Spatial placement or relative offset.
- `distance` — Size, reach, or distance tuning.
- `height` — Field of type `float` used by systems that consume this component.
- `pitchDeg` — Field of type `float` used by systems that consume this component.
- `minPitchDeg` — Field of type `float` used by systems that consume this component.
- `maxPitchDeg` — Field of type `float` used by systems that consume this component.
- `yawDeg` — Field of type `float` used by systems that consume this component.
- `followSharpness` — Field of type `float` used by systems that consume this component.

## Example

```cpp
auto& follow = registry.emplace<ecs::ThirdPersonCameraComponent>(entity);
follow.target = player;
follow.distance = 4.8f;
follow.height = 1.1f;
```

## Notes

- Pair this with a `CameraComponent` on the same entity or an active camera entity in your scene setup.
