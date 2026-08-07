# Terminal Commands

Author: Karl-Johan Bailey

Use these commands while iterating on the game locally.

## Build and rebuild

```bash
cmake -S . -B build
cmake --build build
```

What this does:

- configures the project into `build/`
- compiles the game binary
- reuses the same build directory across edits for faster rebuilds

When to use it:

- after changing C++ source files
- after adding a new component, factory, or service source file
- after editing docs only if you want to keep the repo state tidy and then rebuild the game

## Run the game

```bash
./build/duppy gameplay
```

Use this when you want the normal gameplay scene with the current ECS and renderer pipeline.

## Run fullscreen at a higher refresh target

```bash
./build/duppy gameplay --fullscreen --refresh-rate 144 --no-vsync
```

Use this when your display supports a higher refresh rate and you want to verify the engine is not clamping itself to 60 FPS.

## Debug and uncapped modes

```bash
./build/duppy gameplay --debug-overlay
./build/duppy gameplay --debug-world
./build/duppy gameplay --debug
./build/duppy gameplay --fps 144 --uncapped
./build/duppy gameplay --fps 144 --uncapped --dev
./tools/dev-gameplay.sh
```

Flag guidance:

- `--debug-overlay` enables the on-screen frame debugger and overlay timings
- `--debug-world` enables world-space debug drawing such as rays, collision boxes, combat boxes, and skeleton lines
- `--debug` enables both overlay and world debug together
- `--fps` sets the desired target frame rate
- `--uncapped` removes the frame cap when you want to measure raw engine performance
- `--dev` hot-reloads runtime content like shaders, textures, models, and world config while the game is running
- `--refresh-rate` should match your monitor if fullscreen is locking to a lower value than expected
- `./tools/dev-gameplay.sh` watches C++ source and rebuilds/relaunches automatically when code changes

Performance note:

- Use `--debug-overlay` when you want the cleanest performance numbers.
- Use `--debug-world` only when you are inspecting spatial queries or collision shapes.
- `--debug` is useful for full inspection, but it can be slower because the extra line rendering and debug geometry add work.
- Read `gpu_ms` as approximate frame GPU time. If it stays much lower than `render_ms`, the render thread is still spending meaningful time on CPU-side setup.

## Inspect content wiring

```bash
rg -n 'registerFactory\("' src/ecs/factories/FactoryKeyService.cpp
rg -n 'factory"|coord"|loadProximityMeters|unloadProximityMeters' assets/world src/ecs/services
rg -n 'HudComponent|BillboardComponent|HudSystem|textureEnabled|showValueText|animatedTexture' src
```

Use these searches when you need to see how a component, factory key, or chunk entry is wired into the runtime.

## Common development flow

1. Edit a component, factory, or service.
2. Update its matching doc page under `docs/`.
3. Run `./tools/dev-gameplay.sh` if you want source edits to rebuild automatically, or rebuild manually with `cmake --build build`.
4. Use `./build/duppy gameplay --dev` when you only need runtime asset/config hot reload.
5. Verify the content behaves as expected.
