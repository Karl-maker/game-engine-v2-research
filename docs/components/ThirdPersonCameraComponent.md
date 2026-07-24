# ThirdPersonCameraComponent

## What it is

- ThirdPersonCameraComponent stores data that systems and factories read to drive camera and view.

## When to use it

- Use this when the camera should follow a target entity from behind or over the shoulder.

## How it fits

- Treat `ThirdPersonCameraComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Bind `target` to the actor you want to follow.
- Tune `distance`, `height`, and pitch before changing smoothing.
- Use `followSharpness` to decide how snappy or damped the camera feels.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `target` — Reference to another entity or a linked runtime object.
- `targetOffset` — Spatial placement or directional tuning.
- `distance` — Size, reach, or distance tuning.
- `height` — Size, reach, or distance tuning.
- `pitchDeg` — Orientation or angular tuning.
- `minPitchDeg` — Orientation or angular tuning.
- `maxPitchDeg` — Orientation or angular tuning.
- `yawDeg` — Orientation or angular tuning.
- `followSharpness` — Field of type `float` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::ThirdPersonCameraComponent>(entity);
component = ecs::ThirdPersonCameraComponent{};
```

## Common pairings

- Pair this with `CameraComponent` on the same scene or camera entity.

## Notes

- Keep `ThirdPersonCameraComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
