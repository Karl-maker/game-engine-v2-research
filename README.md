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
./build/duppy
```

## Build & Run (single command, no CMake)

macOS/Linux:
```bash
mkdir -p build &&
g++ -std=c++17 -O2 -pthread -Isrc \
  src/main.cpp src/core/*.cpp src/ecs/EntityRegistry.cpp \
  -o build/duppy &&
./build/duppy
```

Windows (Developer Command Prompt):
```bat
mkdir build
cl /std:c++17 /W4 /EHsc /I src ^
  src\main.cpp src\core\*.cpp src\ecs\EntityRegistry.cpp ^
  /Fe:build\duppy.exe
build\duppy.exe
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

Key ideas:
- `createEntity(name)` allocates a monotonic `EntityId` (+1) and attaches `IdentityComponent`.
- Components are attached type-safely: `registry.emplace<MyComponent>(id, ...)`.
- Fast filtering: `registry.view<A, B>([](EntityId id, A& a, B& b) { ... });`
- Safe edits from many systems: queue changes with `defer...` and call `applyDeferred()` once per frame.

Usage examples:
- `docs/entity-registry.md`

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
