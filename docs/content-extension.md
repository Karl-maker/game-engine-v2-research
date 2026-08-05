# Content Extension Guide

Author: Karl-Johan Bailey

This guide explains how to extend the project without wiring everything by hand.

For detailed reference, open the per-component, per-factory, per-service, and terminal docs in `docs/README.md`.

## 1) Entity components

Keep components data-only when possible. That keeps them easy to save, stream, and reason about.

Recommended flow:

1. Add a component in `src/ecs/components/`.
2. Add a system that consumes it.
3. Add a factory or gameplay setup that creates it.
4. If the component should be data-driven, expose it through JSON parsing.

Examples already in the tree:

- `TransformComponent` for position/rotation/scale
- `TerrainComponent` for terrain shape and LOD
- `VfxComponent` for fire/electricity/sparks
- `SkyComponent` for visual atmosphere settings

## 2) Factory system

Factories are how chunks become entities.

Current flow:

- `ChunkStreamingService` reads a chunk definition
- `FileChunkSource` returns JSON data
- `EntityFactoryRegistry` maps `"factory"` keys to `IEntityFactory`
- each factory creates entities from JSON config

To add a new factory:

1. Create a new factory config and implementation in `src/ecs/factories/`.
2. Parse the JSON shape you want in the factory implementation.
3. Register the key in `FactoryKeyService.cpp`.
4. Add the file to `CMakeLists.txt`.
5. Add a test chunk entry in `assets/world/chunks_demo.json`.

Example chunk entry:

```json
{
  "factory": "vfx",
  "config": {
    "name": "campfire_vfx",
    "type": "Fire",
    "positionLocal": [2.0, 0.0, 3.0],
    "spawnRate": 30.0
  }
}
```

## 3) Chunk streaming

Chunk coordinates are on the X/Z plane:

- `coord: [0, 0]` is the origin
- `coord: [0, 1]` is the next chunk forward
- `coord: [1, 0]` is one chunk to the right

Chunks load when the player is close enough and unload when they are too far away.

Important settings:

- `chunkSizeMeters`
- `searchRadiusChunks`
- `loadProximityMeters`
- `unloadProximityMeters`

For broad quality/performance control, prefer the `persistent.performance`, `persistent.profiling`, and `persistent.render`
blocks in `assets/world/config.json` before hand-editing every chunk entry. The preset sets sane defaults, the profiling block
controls the overlay, and the render block can disable expensive feature families like grass, VFX, billboards, HUD, or post.

## 4) Meshes and model assets

Meshes are loaded by `MeshAssetService`.

What a loader should provide:

- vertex and index buffers
- material names
- optional skeleton data
- optional animation clips

To add a format:

1. Implement `assets::IMeshAssetLoader`.
2. Register the loader by file extension.
3. Return a populated `LoadedMeshAsset`.
4. Point `MeshComponent::meshData.key` at the file.

Example:

```cpp
auto& mesh = registry.emplace<ecs::MeshComponent>(character);
mesh.meshData.enabled = true;
mesh.meshData.key = "assets/models/business-man/scene.gltf";
```

Mesh material override tips:

- add `ShaderComponent.textures` entries like `albedo`, `normalgl`, `roughness`, `metallic`, `ao`, `specular`, `emissive`, `displacement`, `metallicRoughness`, or `orm`
- add `ShaderComponent.parameters` such as `roughness`, `metallic`, `specularIntensity`, `normalScale`, `aoStrength`, `emissiveColor`, `emissiveStrength`, or `displacementStrength`
- add `ShaderComponent.lodBreakpoints` when you want those textures or parameters to swap by camera distance
- set `overrideTessellation = true` inside a breakpoint when you also want a different tessellation band or quality at that distance
- for chunk-authored actors/combatants/objects you can now use a higher-level `material` block with `maps`, `textures`, `values`, or `shading` keys; common aliases like `albedo`, `normal`, `metallicRoughness`, `displacement`, `roughness`, `normalScale`, `aoStrength`, `displacementStrength`, `tessNear`, `tessFar`, `tessMin`, `tessMax`, `tessQuality`, and `sinkStrength` are converted into the lower-level shader bindings automatically
- glTF files already auto-import base color, normal, metallic-roughness, occlusion, emissive, and `KHR_materials_specular` data when those channels exist in the file
- rely on asset UVs for mapping; UV coordinates are loaded from the mesh file when they exist
- if the asset has no UVs, the engine does not auto-generate a new unwrap during load

Combat and animation authoring tips:

- add `animation.clips` entries with `{ "key": "...", "clip": "..." }` so gameplay can refer to stable keys like `idle`, `walk`, `run`, or `attack_light` instead of raw asset clip names
- add `animation.locomotion` to map the automatic idle/walk/run selection onto those keys
- add `pose.poses` for reusable named bone overrides that can be blended by weight
- add `combat.volumes` to spawn many hurt or hit volumes on the parent, a specific bone, a socket, or another mesh target
- add `combat.raycasts` when you want traces and combat volumes to work together; combat impact events now record whether there was ray support and whether a physics collision happened
- add `combat.vfx` for attached high-quality fire, electricity, smoke, steam, or sparks emitters with extra controls such as `glowStrength`, `emberRate`, `smokeAmount`, `arcThickness`, and `sparkGravityScale`

## 5) Materials

Material presets live in `src/materials/presets/`.

Use them when:

- you want a consistent terrain look
- you want a reusable sky setup
- you want a starting point for a new environment material

To create a new preset:

1. Copy an existing preset header.
2. Update the texture slots.
3. Set shader parameters.
4. Return a `render::ShaderComponent`.

## 6) HUD widgets

HUD widgets are defined with `ecs::HudComponent`.

Use cases:

- screen-space life bars and ammo counters
- world-space nameplates and floating health values
- textured UI panels, portraits, icons, and frames

Widget fields that matter most:

- `space` — `Screen` or `World`
- `kind` — `Panel`, `Image`, `Text`, or `Bar`
- `positionPx` / `sizePx` — screen-space layout in pixels
- `worldOffset` / `sizeMeters` — world-space layout in meters
- `textureEnabled` and `texture` — use an image on the widget
- `animatedTexture` — cycle a frame sequence for lightweight video-style playback
- `text`, `label`, and `showValueText` — dynamic text
- `sourceEntity` and `valueSource` — bind to gameplay state such as health

Example life bar:

```cpp
auto& hud = registry.emplace<ecs::HudComponent>(hudEntity);

ecs::HudComponent::Widget lifeBar;
lifeBar.name = "life_bar";
lifeBar.kind = ecs::HudComponent::Kind::Bar;
lifeBar.space = ecs::HudComponent::Space::Screen;
lifeBar.positionPx = {96.0f, 28.0f};
lifeBar.sizePx = {320.0f, 24.0f};
lifeBar.sourceEntity = player;
lifeBar.valueSource = ecs::HudComponent::ValueSource::StatsHealth;
lifeBar.label = "Life";
lifeBar.showValueText = true;
hud.widgets.push_back(lifeBar);
```

Texture tips:

- use `Kind::Image` for a portrait or icon
- use `Kind::Panel` for a textured frame
- use `Kind::Bar` if you want a colored fill over a panel
- set `textureEnabled = true` and point `texture.key` at an image path
- fill `animatedTexture.frames` when the widget should cycle multiple images over time

Dynamic text tips:

- set `text` directly for custom runtime strings
- set `showValueText = true` to append a numeric readout
- update the component each frame if the displayed text changes often

World-space tips:

- set `space = ecs::HudComponent::Space::World`
- set `billboard = true` to face the camera
- use `worldOffset` to float the widget above the actor
- bind the widget to the player or NPC with `sourceEntity`

## 7) Billboards

Use `ecs::BillboardComponent` for simple world-space textured quads.

- set `faceMode = ecs::BillboardComponent::FaceMode::CameraPlane` for full camera-facing quads
- set `faceMode = ecs::BillboardComponent::FaceMode::YawOnly` for upright signs or markers
- assign `texture` for a single image
- fill `animatedTexture.frames` for extracted video frames or flipbook playback
- tune `sizeMeters`, `pivot`, and `maxRenderDistance` for the final presentation
- use transparent PNGs directly and fade the full card with `tint.a` or `alpha`

Config-driven billboards can be authored directly in `assets/world/config.json`:

```json
{
  "factory": "billboard",
  "config": {
    "name": "animated_billboard_example",
    "positionLocal": [7.0, 53.0, 6.0],
    "faceMode": "cameraPlane",
    "sizeMeters": [2.6, 2.6],
    "pivot": [0.5, 0.1],
    "maxRenderDistance": 120.0,
    "doubleSided": true,
    "depthWrite": false,
    "textureEnabled": true,
    "texture": "assets/hud/combatant-health.png",
    "tint": [1.0, 1.0, 1.0, 0.78],
    "animatedTexture": {
      "enabled": true,
      "framesPerSecond": 2.25,
      "looping": true,
      "pingPong": true,
      "holdLastFrame": true,
      "frames": [
        "assets/hud/combatant-health.png",
        "assets/hud/player-health.png"
      ]
    }
  }
}
```

Useful `billboard` config fields:

- `faceMode`: `cameraPlane`, `yawOnly`, or `none`
- `sizeMeters`: `[width, height]`
- `pivot`: normalized anchor point
- `worldOffset` and `rotationOffsetDeg`: extra placement control
- `texture`: base image
- `animatedTexture.frames`: image sequence
- `animatedTexture.framesPerSecond`, `looping`, `pingPong`, `holdLastFrame`, `startFrame`: playback behavior
- `tint` and `alpha`: transparency and color multiplier

## 8) Useful terminal commands

Build:

```bash
cmake -S . -B build
cmake --build build
```

Run gameplay:

```bash
./build/duppy gameplay
```

Run fullscreen at a high refresh target:

```bash
./build/duppy gameplay --fullscreen --refresh-rate 144 --no-vsync
```

Inspect registered factory keys:

```bash
rg -n 'registerFactory\("' src/ecs/factories/FactoryKeyService.cpp
```

Inspect chunk data:

```bash
cat assets/world/chunks_demo.json
```

Inspect mesh loader registration:

```bash
rg -n 'registerLoader|makeGltfMeshLoader|IMeshAssetLoader' src/assets src/graphics
```

Inspect HUD and billboard rendering:

```bash
rg -n 'HudComponent|BillboardComponent|HudSystem|m_hudProgram|showValueText|textureEnabled|animatedTexture' src
```
