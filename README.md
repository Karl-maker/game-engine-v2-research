# Duppy Conquerer (Base Loop)

Author: Karl-Johan Bailey

Minimal, dependency-free C++ game loop scaffold:
- Delta time + elapsed time
- Frame cap / target FPS
- Basic console input (type a command and press Enter)
- Debug display configuration (FPS, delta, frame index, last input)

## Build & Run (CMake)

```bash
cmake -S . -B build
cmake --build build
./build/duppy attachment
```

## Build & Run (single command, no CMake)

macOS/Linux:
```bash
mkdir -p build &&
g++ -std=c++17 -O2 -pthread -Isrc \
  src/main.cpp src/core/*.cpp src/ecs/EntityRegistry.cpp src/ecs/systems/*.cpp src/games/*.cpp \
  -o build/duppy &&
./build/duppy attachment
```

Windows (Developer Command Prompt):
```bat
mkdir build
cl /std:c++17 /W4 /EHsc /I src ^
  src\main.cpp src\core\*.cpp src\ecs\EntityRegistry.cpp src\ecs\systems\*.cpp src\games\*.cpp ^
  /Fe:build\duppy.exe
build\duppy.exe attachment
```

## Controls

- Type `q` then press Enter to quit.
- Any other line is treated as a simple input event.

## Notes

- The console input implementation uses a background thread that blocks on `std::getline`. This keeps the main loop non-blocking without platform-specific APIs.

## Entity Register Service (ECS)

Files:
- `src/ecs/EntityRegistry.h`
- `src/ecs/EntityRegistry.cpp`
- `src/ecs/components/IdentityComponent.h`
- `src/ecs/components/TransformComponent.h`
- `src/ecs/components/AttachmentComponent.h`
- `src/ecs/components/TargetComponent.h`
- `src/ecs/components/PhysicsMaterialComponent.h`
- `src/ecs/components/LightComponent.h`
- `src/ecs/components/CameraComponent.h`
- `src/ecs/components/RigidbodyComponent.h`
- `src/ecs/components/CombatVolumeComponent.h`
- `src/ecs/components/StatsComponent.h`
- `src/ecs/components/RaycastComponent.h`
- `src/ecs/services/RaycastConeFactoryService.h`
- `src/ecs/components/MotionComponent.h`
- `src/ecs/components/ControllerComponent.h`
- `src/ecs/components/TerrainComponent.h`
- `src/ecs/components/ColliderComponent.h`
- `src/ecs/components/MeshRendererComponent.h`

Key ideas:
- `createEntity(name)` allocates a monotonic `EntityId` (+1) and attaches `IdentityComponent`.
- Components are attached type-safely: `registry.emplace<MyComponent>(id, ...)`.
- Fast filtering: `registry.view<A, B>([](EntityId id, A& a, B& b) { ... });`
- Safe edits from many systems: queue changes with `defer...` and call `applyDeferred()` once per frame.

Usage examples:
- `docs/entity-registry.md`

## Demos (terminal)

```bash
./build/duppy attachment
./build/duppy target
```

### Components

These are the representations that we can store in a database or config file to use in our game to create the world.

### Identity Component

Every entity has:
- `id` (monotonic +1 allocation)
- `name`

### Transform Component

Any element can have a location in the world. Attach `TransformComponent` to entities so they can have `id`, `name`, and transform details.

Fields:
- Position: `x`, `y`, `z`
- Rotation: `pitch`, `yaw`, `roll`
- Scale: `x`, `y`, `z`

Example:
```cpp
#include "ecs/components/TransformComponent.h"

auto id = registry.createEntity("Crate");
registry.emplace<ecs::TransformComponent>(id);  // add
registry.remove<ecs::TransformComponent>(id);   // remove
```

### Attachment Component

Describes how one entity is attached to another (data only; a system interprets it).

Example:
```cpp
#include "ecs/components/AttachmentComponent.h"

registry.emplace<ecs::AttachmentComponent>(camera);
```

### Target Component

Orientation-only descriptor: rotate an entity to face a target entity (data only; a system interprets it).

Example:
```cpp
#include "ecs/components/TargetComponent.h"

registry.emplace<ecs::TargetComponent>(camera).targetEntity = player;
```
