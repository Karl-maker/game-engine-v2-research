# Content Extension Guide

Author: Karl-Johan Bailey

This guide explains how to extend the project without wiring everything by hand.

For deeper reference, open the per-component, per-factory, per-service, and terminal docs in `docs/README.md`.

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
2. Parse the JSON shape you want in `FactoryKeyService.cpp`.
3. Register the key in `registerFactoriesFromEcsFactoriesDir(...)`.
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
auto& mesh = registry.emplace<ecs::MeshComponent>(entity);
mesh.meshData.enabled = true;
mesh.meshData.key = "assets/models/business-man/scene.gltf";
```

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

Dynamic text tips:

- set `text` directly for custom runtime strings
- set `showValueText = true` to append a numeric readout
- update the component each frame if the displayed text changes often

World-space tips:

- set `space = ecs::HudComponent::Space::World`
- set `billboard = true` to face the camera
- use `worldOffset` to float the widget above the actor
- bind the widget to the player or NPC with `sourceEntity`

## 7) Useful terminal commands

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
rg -n 'registerFactory\\("' src/ecs/factories/FactoryKeyService.cpp
```

Inspect chunk data:

```bash
cat assets/world/chunks_demo.json
```

Inspect mesh loader registration:

```bash
rg -n 'registerLoader|makeGltfMeshLoader|IMeshAssetLoader' src/assets src/graphics
```

Inspect HUD rendering and bindings:

```bash
rg -n 'HudComponent|HudSystem|m_hudProgram|showValueText|textureEnabled' src
```
