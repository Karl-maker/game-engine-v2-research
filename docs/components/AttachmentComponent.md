# AttachmentComponent

## Purpose

- AttachmentComponent
- Describes how one entity is attached to (or driven by) another entity.
- Example idea:
- - Entity `main_character` has an AttachmentComponent that targets `camera_01`
- - A system interprets these descriptors each frame and updates transforms accordingly
- This component is intentionally descriptive only:
- - It stores references + settings.
- - It does NOT apply transforms by itself (that is system logic).

## Use when

- Use `AttachmentComponent` when you need ECS data for attachmentcomponent behavior.

## Enums

- `Mode`: `Parent`, `// Hard parent: child matches parent (optionally with offsets).
    Follow`, `// Smoothly follow target position and/or rotation.
    Orbit`, `// Orbit around target using yaw/pitch + radius.
    LookAt`, `// Rotate to look at target (optionally with offsets).
    Socket`, `// Attach to a named socket/bone on the target (string key).
    Spring`, `// Spring-damped follow (e.g. camera boom).`
- `Space`: `Local`, `// Offsets interpreted in target local space.
    World`, `// Offsets interpreted in world space.`

## Key fields

- `targetEntity` — Reference to another entity or input source.
- `mode` — Field of type `Mode` used by systems that consume this component.
- `enabled` — Enable or disable this part of the component.
- `positionOffset` — Spatial placement or relative offset.
- `rotationOffset` — Spatial placement or relative offset.
- `scaleOffset` — Spatial placement or relative offset.
- `inheritPosition` — Spatial placement or relative offset.
- `inheritRotation` — Orientation or angle tuning.
- `inheritScale` — Size, reach, or distance tuning.
- `space` — Field of type `Space` used by systems that consume this component.
- `positionSpeed` — Spatial placement or relative offset.
- `rotationSpeed` — Orientation or angle tuning.
- `interpolationSpeed` — Movement or force tuning.
- `socketKey` — Field of type `std::string` used by systems that consume this component.
- `orbitRadius` — Size, reach, or distance tuning.
- `minPitchDeg` — Field of type `float` used by systems that consume this component.
- `maxPitchDeg` — Field of type `float` used by systems that consume this component.
- `currentYawDeg` — Field of type `float` used by systems that consume this component.
- `currentPitchDeg` — Field of type `float` used by systems that consume this component.
- `attachments` — Field of type `std::vector<Attachment>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::AttachmentComponent>(entity);
component = ecs::AttachmentComponent{};
```

## Notes

- Keep `AttachmentComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
