# CameraComponent

## Purpose

- CameraComponent (descriptive only)
- Stores camera settings. A camera orchestrator (outside this component) decides which camera is active.

## Use when

- Use this on any camera entity that should drive projection, frustum settings, depth of field, or motion blur.

## Enums

- `ProjectionType`: `Perspective`, `Orthographic`
- `FocusMode`: `ManualDistance`, `TargetEntity`

## Key fields

- `projectionType` — Field of type `ProjectionType` used by systems that consume this component.
- `fieldOfViewDeg` — Field of type `float` used by systems that consume this component.
- `orthographicSize` — Size, reach, or distance tuning.
- `nearClipPlane` — Field of type `float` used by systems that consume this component.
- `farClipPlane` — Field of type `float` used by systems that consume this component.
- `aspectRatio` — Field of type `float` used by systems that consume this component.
- `useFramebufferAspectRatio` — Field of type `bool` used by systems that consume this component.
- `visibleLayers` — Layer or filtering control.
- `frustumCulling` — Field of type `bool` used by systems that consume this component.
- `occlusionCulling` — Field of type `bool` used by systems that consume this component.
- `shadowDistance` — Size, reach, or distance tuning.
- `lodBias` — Level-of-detail or quality control.
- `renderScale` — Size, reach, or distance tuning.
- `enabled` — Enable or disable this part of the component.
- `focusMode` — Reference to another entity or input source.
- `focusDistance` — Size, reach, or distance tuning.
- `focusRange` — Reference to another entity or input source.
- `blurStrength` — Field of type `float` used by systems that consume this component.
- `focusTarget` — Reference to another entity or input source.
- `focusTargetOffset` — Spatial placement or relative offset.
- `strength` — Field of type `float` used by systems that consume this component.
- `maxBlurPixels` — Field of type `float` used by systems that consume this component.
- `samples` — Field of type `int` used by systems that consume this component.
- `depthOfField` — Field of type `DepthOfFieldSettings` used by systems that consume this component.
- `motionBlur` — Field of type `MotionBlurSettings` used by systems that consume this component.

## Example

```cpp
auto& camera = registry.emplace<ecs::CameraComponent>(entity);
camera.projectionType = ecs::CameraComponent::ProjectionType::Perspective;
camera.fieldOfViewDeg = 60.0f;
camera.depthOfField.enabled = true;
camera.depthOfField.focusMode = ecs::CameraComponent::DepthOfFieldSettings::FocusMode::TargetEntity;
camera.motionBlur.enabled = true;
```

## Notes

- The renderer or camera controller decides which camera is active.
- Use `focusTarget` plus `focusTargetOffset` for a camera that follows an actor or target point.
- Use `motionBlur` together with the frame pacing settings when you want a cinematic feel.
