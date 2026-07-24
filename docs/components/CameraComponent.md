# CameraComponent

## What it is

- CameraComponent (descriptive only)
- Stores camera settings. A camera orchestrator (outside this component) decides which camera is active.

## When to use it

- Use this on the active camera entity that controls the current view.

## How it fits

- Treat `CameraComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Choose perspective or orthographic projection first.
- Set clip planes before tuning FOV or orthographic size.
- Configure depth of field and motion blur only after the camera framing feels correct.

## Camera recipes

- **Static free camera:** use `ProjectionType::Perspective`, leave `depthOfField` and `motionBlur` disabled, and tune clip planes for your scene size.
- **Follow camera:** pair this with `ThirdPersonCameraComponent`, then let the follow system drive the transform while this component controls projection and post effects.
- **Cinematic camera:** enable depth of field, point `focusTarget` at the subject, and keep `motionBlur.samples` modest so the frame rate stays stable.

## Focus settings

The depth-of-field block is the part that controls autofocus and blur:

- `enabled` turns the effect on or off.
- `focusMode` chooses either a manual distance or a target entity.
- `focusDistance` and `focusRange` define where sharp focus begins and how quickly blur ramps in.
- `blurStrength` controls how soft the out-of-focus area becomes.
- `focusTarget` and `focusTargetOffset` let the camera focus on an entity instead of a fixed distance.

## Motion blur settings

The motion blur block controls cinematic streaking:

- `enabled` turns the effect on or off.
- `strength` scales the overall blur amount.
- `maxBlurPixels` clamps how large a blur streak can become.
- `samples` trades quality for cost.

## Enums

- `ProjectionType`: `Perspective`, `Orthographic`
- `FocusMode`: `ManualDistance`, `TargetEntity`

## Field guide

- `projectionType` — Enum or bitmask value that changes system behavior.
- `fieldOfViewDeg` — Field of type `float` consumed by systems that read this component.
- `orthographicSize` — Size, reach, or distance tuning.
- `nearClipPlane` — Stable reference used by content, loaders, or rendering systems.
- `farClipPlane` — Stable reference used by content, loaders, or rendering systems.
- `aspectRatio` — Field of type `float` consumed by systems that read this component.
- `useFramebufferAspectRatio` — Field of type `bool` consumed by systems that read this component.
- `visibleLayers` — Enum or bitmask value that changes system behavior.
- `frustumCulling` — Master on/off switch or similar behavior flag.
- `occlusionCulling` — Master on/off switch or similar behavior flag.
- `shadowDistance` — Size, reach, or distance tuning.
- `lodBias` — Field of type `float` consumed by systems that read this component.
- `renderScale` — Size, reach, or distance tuning.
- `enabled` — Master on/off switch or similar behavior flag.
- `focusMode` — Orientation or angular tuning.
- `focusDistance` — Orientation or angular tuning.
- `focusRange` — Orientation or angular tuning.
- `blurStrength` — Numeric tuning used by gameplay or rendering systems.
- `focusTarget` — Orientation or angular tuning.
- `focusTargetOffset` — Spatial placement or directional tuning.
- `strength` — Numeric tuning used by gameplay or rendering systems.
- `maxBlurPixels` — Field of type `float` consumed by systems that read this component.
- `samples` — Numeric tuning used by gameplay or rendering systems.
- `depthOfField` — Field of type `DepthOfFieldSettings` consumed by systems that read this component.
- `motionBlur` — Field of type `MotionBlurSettings` consumed by systems that read this component.

## Example

```cpp
auto& camera = registry.emplace<ecs::CameraComponent>(entity);
camera.projectionType = ecs::CameraComponent::ProjectionType::Perspective;
camera.fieldOfViewDeg = 60.0f;
camera.depthOfField.enabled = true;
camera.depthOfField.focusMode = ecs::CameraComponent::DepthOfFieldSettings::FocusMode::TargetEntity;
camera.motionBlur.enabled = true;
camera.motionBlur.samples = 12;
```

## Common pairings

- Use `useFramebufferAspectRatio` when the camera should follow the actual render target shape.
- Use `focusTarget` plus `focusTargetOffset` when you want autofocus around another entity.
- Motion blur should be paired with a sensible frame cap or refresh target.

## Notes

- Keep `CameraComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
