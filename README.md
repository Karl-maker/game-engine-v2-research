# Duppy Conquerer

Author: Karl-Johan Bailey

This project is a C++ ECS game scaffold with:
- a deterministic main loop
- a data-driven entity/factory system
- chunk streaming from JSON
- async mesh loading
- OpenGL rendering with terrain, grass, rocks, sky, fog, motion blur, and depth of field

Detailed reference docs live in `docs/README.md` and are split into per-component, per-factory, per-service, and terminal-command pages.

## Quick Start

Build and run:

```bash
cmake -S . -B build
cmake --build build
./build/duppy gameplay
```

Common runtime flags:

```bash
./build/duppy gameplay --fullscreen --refresh-rate 144 --no-vsync
./build/duppy gameplay --chunks assets/world/chunks_demo.json --chunk-load-proximity 18 --chunk-unload-proximity 30
./build/duppy gameplay --fps 144 --uncapped
./build/duppy gameplay --debug-overlay
./build/duppy gameplay --debug-world
./build/duppy gameplay --debug
```

Single-command build examples:

```bash
mkdir -p build &&
g++ -std=c++17 -O2 -pthread -Isrc \
  src/main.cpp src/core/*.cpp src/ecs/EntityRegistry.cpp src/ecs/systems/*.cpp src/ecs/factories/*.cpp src/ecs/services/*.cpp src/assets/MeshAssetService.cpp src/data/Json.cpp src/games/*.cpp \
  -o build/duppy
```

```bat
mkdir build
cl /std:c++17 /W4 /EHsc /I src ^
  src\main.cpp src\core\*.cpp src\ecs\EntityRegistry.cpp src\ecs\systems\*.cpp src\ecs\factories\*.cpp src\ecs\services\*.cpp src\assets\MeshAssetService.cpp src\data\Json.cpp src\games\*.cpp ^
  /Fe:build\duppy.exe
```

## Runtime Commands

Type these into the terminal input while the game is running:

- `q`, `quit`, `exit` — quit the game
- any other line — passed into the game loop as a text input event

Useful launch commands:

- `./build/duppy attachment`
- `./build/duppy target`
- `./build/duppy enemy`
- `./build/duppy gameplay`

If fullscreen is stuck at 60 FPS on your display, make sure you are not forcing vsync and use `--refresh-rate 144` or your panel’s native refresh rate.

For frame profiling, prefer `--debug-overlay` first. It shows the frame debugger without enabling the heavier world-space debug lines, so the numbers are much more trustworthy.

The overlay and title bar now also show `gpu_ms`, which helps separate CPU-side render setup cost from actual GPU frame time.

## Project Map

- `src/main.cpp` — CLI flags, mode selection, loop config
- `src/core/` — loop, timing, input, quit handling
- `src/ecs/` — components, systems, factories, and services
- `src/graphics/` — OpenGL renderer and shader service
- `src/assets/` — async mesh loading
- `src/materials/` — reusable material presets
- `assets/world/` — chunk config examples
- `graphics/shaders/` — renderer shaders

## Useful Dev Commands

Find built-in factory keys:

```bash
rg -n 'registerFactory\\("' src/ecs/factories/FactoryKeyService.cpp
```

Find chunk examples and proximity settings:

```bash
rg -n '"factory"|"coord"|"loadProximityMeters"|"unloadProximityMeters"' assets/world src/ecs/services
```

Find mesh loader hooks:

```bash
rg -n 'registerLoader|IMeshAssetLoader|makeGltfMeshLoader' src/assets src/graphics
```

Find material presets:

```bash
find src/materials/presets -name '*.h' | sort
```

For step-by-step extension help, open `docs/content-extension.md`, `docs/components.md`, `docs/factories.md`, `docs/services.md`, and `docs/terminal.md`.

## ECS Components

Components are data only. Systems read them and decide how to simulate or render them.

### Identity and Transform

- `IdentityComponent` — entity id/name metadata
- `TransformComponent` — world position, rotation, scale
- `HierarchyComponent` — parent/child relationships
- `AttachmentComponent` — attach one entity to another
- `TargetComponent` — face or track a target entity

### Motion and Physics

- `MotionComponent` — high-level movement intent/state
- `ControllerComponent` — gameplay control actions
- `StatsComponent` — movement and combat tuning
- `RigidbodyComponent` — mass, gravity, velocity, physics response
- `ColliderComponent` — shape and collision settings
- `PhysicsMaterialComponent` — friction/bounce-style tuning

### Character and Animation

- `CharacterComponent` — character identity/behavior marker
- `AnimationComponent` — animation clips and playback
- `SkeletonComponent` — bones and inverse bind data
- `PoseComponent` — runtime bone pose state
- `IKComponent` — inverse kinematics chains
- `SocketComponent` — attach points on bones
- `BoneKeyComponent` — bone lookup helpers

### Camera and View

- `CameraComponent` — projection, FOV, DOF, motion blur
- `ThirdPersonCameraComponent` — follow camera settings

### Rendering and World

- `BillboardComponent` — world-space textured quads with camera-facing modes and animated frame playback
- `MeshComponent` — mesh asset reference and visibility
- `MeshRendererComponent` — render-oriented mesh state
- `ShaderComponent` — shader key, textures, parameters
- `RenderSettingsComponent` — global render toggles
- `LightComponent` — directional/point/spot lighting
- `SkyComponent` — procedural sky and cloud settings
- `FogVolumeComponent` — volumetric fog box
- `TerrainComponent` — terrain mesh + terrain LOD controls
- `GrassPatchComponent` — grass and vegetation instancing
- `RockScatterComponent` — rock scatter instancing
- `HudComponent` — screen-space and world-space HUD widgets
- `CombatVolumeComponent` — hit/hurt box data
- `AudioComponent` — audio references and playback settings
- `VfxComponent` — fire, sparks, electricity, smoke, steam emitters

### Sensors and Queries

- `SensorComponent` — proximity/visibility sensing
- `RaycastComponent` — ray query data and debug draw settings

## Services

Services own reusable engine behavior.

- `EntityRegistry` — creates entities, stores components, filters views
- `EventService` — queues and consumes gameplay events
- `ChunkStreamingService` — loads and unloads chunk entities by proximity
- `IChunkSource` — pluggable chunk data provider
- `FileChunkSource` — JSON file-backed chunk source
- `EntityFactoryRegistry` — maps string keys to factories
- `IEntityFactory` — data-driven entity creation interface
- `FactoryKeyService` — registers the built-in factory keys
- `HudSystem` — updates HUD values such as life bars from gameplay state
- `SpatialHashGridService` — broadphase grid for collision and queries
- `IKService` — helper routines for IK chain setup
- `RaycastConeFactoryService` — builds ray sensor setups

## Factory Keys

The current chunk/factory keys live in `src/ecs/factories/FactoryKeyService.cpp`.

- `entity`
- `character`
- `enemy`
- `weapon`
- `building`
- `asset`
- `terrain`
- `vfx`

To add a new factory key:

1. Create a new factory header/cpp in `src/ecs/factories/`.
2. Implement an `IEntityFactory` adapter or a direct factory class.
3. Register it in `registerFactoriesFromEcsFactoriesDir(...)`.
4. Add the new source file to `CMakeLists.txt`.
5. Add a chunk entry that uses the new key.

## Chunks

Chunks are defined in JSON and loaded around the player by `ChunkStreamingService`.

Chunk config shape:

```json
{
  "chunks": [
    {
      "coord": [0, 0],
      "entities": [
        { "factory": "entity", "config": { "name": "spawn_marker" } },
        { "factory": "vfx", "config": { "type": "Fire", "positionLocal": [2, 0, 3] } }
      ]
    }
  ]
}
```

Rules:

- `coord: [x, y]` maps to chunk coordinates on the X/Z plane
- `positionLocal` is relative to the chunk origin
- `position` is world-space
- `factory` must match a registered factory key
- `config` is passed straight into that factory

Streaming settings:

- `chunkSizeMeters` — size of one chunk
- `searchRadiusChunks` — how many chunk coordinates to scan
- `loadProximityMeters` — how close you need to be before a chunk loads
- `unloadProximityMeters` — how far away before a chunk unloads

## Meshes

Mesh loading is handled by `MeshAssetService`.

What it does:

- picks a loader by file extension
- loads meshes asynchronously
- stores submeshes, materials, skeletons, and animations
- lets the renderer request assets by source path

To add a new mesh format:

1. Implement `assets::IMeshAssetLoader`.
2. Register it in `MeshAssetService::start()` or the relevant loader setup.
3. Return a populated `LoadedMeshAsset`.
4. Point a `MeshComponent` at the file path.

Mesh materials can also override shader texture refs through `ShaderComponent.textures` using slots like `albedo`, `normalgl`, `roughness`, `metallic`, `ao`, `specular`, `emissive`, `displacement`, `metallicRoughness`, and `orm`.

The glTF loader now auto-imports base color, normal, metallic-roughness, occlusion, and emissive textures plus their common factors, and it also reads `KHR_materials_specular` when present.

Mesh UVs are loaded from the source asset automatically when they exist; the engine does not generate new UV unwraps during load.

Example:

```cpp
auto& mesh = registry.emplace<ecs::MeshComponent>(entity);
mesh.meshData.enabled = true;
mesh.meshData.key = "assets/models/business-man/scene.gltf";
```

## Materials and Presets

Reusable material presets live in `src/materials/presets/`.

Common presets:

- `Dirt`
- `HighQualityDirt`
- `HighQualityDirtRockLayer`
- `HighQualityDirtRockGrassLayer`
- `Mulch`
- `PebblyDirt`
- `Sand`
- `Stone`
- `StoneGrass`
- `Sky`
- `RealisticSkyClouds`

These presets build `render::ShaderComponent` descriptions with textures and parameters. Use them when you want a new entity to share a known look without hand-wiring every texture.

To create a new preset:

1. Copy an existing preset header in `src/materials/presets/`.
2. Change the texture slots and parameter values.
3. Return a configured `render::ShaderComponent`.
4. Use that preset from a factory, gameplay setup, or chunk config.

Example:

```cpp
auto& shader = registry.emplace<ecs::ShaderComponent>(entity, materials::presets::HighQualityDirtRockLayer());
shader.shader.key = "graphics/shaders/terrain";
```

## HUD / UI

HUD widgets are data-driven and can be rendered:

- in screen space, like a classic UI
- in world space, like a floating health label or nameplate

The demo shows a `HudComponent` with:

- a textured portrait image
- a screen-space life bar driven by `StatsComponent::health`
- a world-space life label above the player

Example:

```cpp
auto& hud = registry.emplace<ecs::HudComponent>(hudEntity);

ecs::HudComponent::Widget lifeBar;
lifeBar.kind = ecs::HudComponent::Kind::Bar;
lifeBar.space = ecs::HudComponent::Space::Screen;
lifeBar.sourceEntity = player;
lifeBar.valueSource = ecs::HudComponent::ValueSource::StatsHealth;
lifeBar.label = "Life";
lifeBar.showValueText = true;
lifeBar.positionPx = {96.0f, 28.0f};
lifeBar.sizePx = {320.0f, 24.0f};
hud.widgets.push_back(lifeBar);
```

Texture usage:

- set `textureEnabled = true`
- assign `texture = {true, "assets/textures/your_image.png", 0}`
- optionally fill `animatedTexture.frames` with extracted video frames or a flipbook sequence
- use `Kind::Image` for a portrait/icon
- use `Kind::Panel` for a textured panel background

Dynamic text:

- set `text` and/or `label`
- set `showValueText = true` to append the live numeric value
- mutate the widget every tick if you want custom runtime text

World-space HUD:

- set `space = HudComponent::Space::World`
- set `billboard = true` for a camera-facing label
- attach the widget to a source entity with `sourceEntity`
- use `worldOffset` to place it above the actor

## Billboards

Billboards are lightweight world-space textured quads.

- Use `BillboardComponent::FaceMode::CameraPlane` for effects or floating cards that should fully face the camera.
- Use `BillboardComponent::FaceMode::YawOnly` for upright signs and markers.
- Use `BillboardComponent::FaceMode::None` when authored transform rotation should stay in control.
- Use `animatedTexture.frames` plus `framesPerSecond` for video-style playback from a frame sequence.

The runtime updater lives in `HudSystem`; the renderer snapshot lives in `GraphicsSystem`; and the actual draw calls happen in `OpenGlRenderer`.

## Adding New ECS Content

### Add a new component

1. Create `src/ecs/components/MyComponent.h`
2. Keep it data-only if possible
3. Add a system that reads it
4. Add a factory or gameplay setup that creates it

### Add a new factory

1. Create a factory class in `src/ecs/factories/`
2. Parse JSON into a config struct in `FactoryKeyService.cpp`
3. Register the key in `registerFactoriesFromEcsFactoriesDir(...)`
4. Update `CMakeLists.txt`
5. Add an example chunk using that key

### Add a new chunk type

1. Pick a chunk coordinate
2. Add entities with `factory` and `config`
3. Use `positionLocal` for content relative to the chunk
4. Let `ChunkStreamingService` load it by proximity

### Add a new mesh or asset type

1. Register a loader in `MeshAssetService`
2. Point a `MeshComponent` at the new file path
3. Add a shader/material preset if needed

### Add a new material preset

1. Copy a file in `src/materials/presets/`
2. Update textures and shader parameters
3. Reuse it from gameplay code or a factory

## Reference Docs

- `docs/entity-registry.md`
- `docs/character-mesh-loading.md`
- `docs/content-extension.md`
