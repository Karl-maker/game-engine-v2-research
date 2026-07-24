# AttachmentComponent

## What it is

- AttachmentComponent
- Describes how one entity is attached to (or driven by) another entity.
- Example idea:
- - Entity `main_character` has an AttachmentComponent that targets `camera_01`
- - A system interprets these descriptors each frame and updates transforms accordingly
- This component is intentionally descriptive only:
- - It stores references + settings.
- - It does NOT apply transforms by itself (that is system logic).

## When to use it

- Use this for weapons, props, cameras, or effects that should stay anchored to another entity.

## How it fits

- Treat `AttachmentComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Point the attachment at the parent entity you want to follow.
- Use the offset fields to place the child relative to the parent.
- Choose whether the child inherits rotation, scale, or both.

## Enums

- `Mode`: `Parent`, `// Hard parent: child matches parent (optionally with offsets).
    Follow`, `// Smoothly follow target position and/or rotation.
    Orbit`, `// Orbit around target using yaw/pitch + radius.
    LookAt`, `// Rotate to look at target (optionally with offsets).
    Socket`, `// Attach to a named socket/bone on the target (string key).
    Spring`, `// Spring-damped follow (e.g. camera boom).`
- `Space`: `Local`, `// Offsets interpreted in target local space.
    World`, `// Offsets interpreted in world space.`

## Field guide

- `targetEntity` — Reference to another entity or a linked runtime object.
- `mode` — Enum or bitmask value that changes system behavior.
- `enabled` — Master on/off switch or similar behavior flag.
- `positionOffset` — Spatial placement or directional tuning.
- `rotationOffset` — Spatial placement or directional tuning.
- `scaleOffset` — Spatial placement or directional tuning.
- `inheritPosition` — Spatial placement or directional tuning.
- `inheritRotation` — Orientation or angular tuning.
- `inheritScale` — Size, reach, or distance tuning.
- `space` — Field of type `Space` consumed by systems that read this component.
- `positionSpeed` — Spatial placement or directional tuning.
- `rotationSpeed` — Orientation or angular tuning.
- `interpolationSpeed` — Numeric tuning used by gameplay or rendering systems.
- `socketKey` — Stable reference used by content, loaders, or rendering systems.
- `orbitRadius` — Size, reach, or distance tuning.
- `minPitchDeg` — Orientation or angular tuning.
- `maxPitchDeg` — Orientation or angular tuning.
- `currentYawDeg` — Orientation or angular tuning.
- `currentPitchDeg` — Orientation or angular tuning.
- `attachments` — Field of type `std::vector<Attachment>` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::AttachmentComponent>(entity);
component = ecs::AttachmentComponent{};
```

## Common pairings

- Use this with `HierarchyComponent` when you need both logical parenting and transform inheritance.

## Notes

- Keep `AttachmentComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
