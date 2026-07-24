# BillboardComponent

## What it is

- BillboardComponent (descriptive only)
- Describes a textured world-space quad that the renderer can rotate toward the active camera.
- Supports single textures and animated frame sequences for video-style playback.

## When to use it

- Use this for floating markers, signs, sprites, holograms, flame cards, spell decals, or simple in-world UI art.

## How it fits

- Treat `BillboardComponent` as data only: create it in a factory or gameplay setup, then let `GraphicsSystem` and the renderer consume it.
- Pair it with `TransformComponent` for placement.

## Facing modes

- `CameraPlane` fully faces the current camera.
- `YawOnly` rotates only around world up, which is better for upright signs, plants, or panels.
- `None` keeps authored orientation from `TransformComponent` plus `rotationOffsetDeg`.

## Texture and playback

- Use `texture` for a single image.
- Turn on `animatedTexture.enabled` and fill `animatedTexture.frames` for frame-cycled playback.
- Use `framesPerSecond` to set playback speed.
- Use `looping`, `pingPong`, and `holdLastFrame` to control how the frame sequence behaves after reaching the end.
- This is ideal for extracted video frames, GIF-like sequences, stylized sprite animations, or cheap VFX cards.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `visible` — Master on/off switch or similar behavior flag.
- `textureEnabled` — Master on/off switch or similar behavior flag.
- `depthWrite` — Rendering-state hint for whether the quad should write depth.
- `doubleSided` — Rendering-state hint for culling.
- `faceMode` — Field of type `FaceMode` consumed by rendering systems.
- `sizeMeters` — Size, reach, or distance tuning.
- `pivot` — Anchor point in normalized quad space where `0,0` is bottom-left and `0.5,0.5` is center.
- `worldOffset` — Spatial placement or directional tuning.
- `rotationOffsetDeg` — Additional orientation applied on top of `TransformComponent` when `faceMode = None`.
- `maxRenderDistance` — Distance culling control.
- `texture` — Stable reference used by content, loaders, or rendering systems.
- `animatedTexture` — Video-style frame playback settings.
- `tint` — Color, tint, or display styling.

## Example

```cpp
auto& tr = registry.emplace<ecs::TransformComponent>(entity);
tr.position = {4.0f, 0.0f, 6.0f};

auto& billboard = registry.emplace<ecs::BillboardComponent>(entity);
billboard.faceMode = ecs::BillboardComponent::FaceMode::YawOnly;
billboard.sizeMeters = {1.4f, 2.2f};
billboard.pivot = {0.5f, 0.0f};
billboard.textureEnabled = true;
billboard.texture = {true, "assets/textures/ground/ground_color.jpg", 0};
billboard.animatedTexture.enabled = true;
billboard.animatedTexture.framesPerSecond = 2.0f;
billboard.animatedTexture.pingPong = true;
billboard.animatedTexture.frames = {
    {true, "assets/textures/ground/ground_color.jpg", 0},
    {true, "assets/textures/stone/stone_color.jpg", 0},
    {true, "assets/textures/dirt/dirt_color.jpg", 0},
};
billboard.tint = {1.0f, 1.0f, 1.0f, 0.95f};
```

## Common pairings

- Use `BillboardComponent` plus `AttachmentComponent` when the quad should follow a socket or actor.
- Use `BillboardComponent` plus `VfxComponent` when you want cheap card-based art near richer particle effects.
- Use `YawOnly` for upright world signage and `CameraPlane` for floating effect cards.

## Notes

- Keep frame sequences short unless the visual really needs them, because each distinct frame is still a texture upload/cache entry.
- If you need actual mesh depth, lighting, or skeletal motion, use a mesh instead of a billboard.
