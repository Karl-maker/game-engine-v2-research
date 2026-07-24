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
./build/duppy gameplay --debug
./build/duppy gameplay --fps 144 --uncapped
```

Flag guidance:

- `--debug` enables extra runtime visibility
- `--fps` sets the desired target frame rate
- `--uncapped` removes the frame cap when you want to measure raw engine performance
- `--refresh-rate` should match your monitor if fullscreen is locking to a lower value than expected

## Inspect content wiring

```bash
rg -n 'registerFactory\("' src/ecs/factories/FactoryKeyService.cpp
rg -n 'factory"|coord"|loadProximityMeters|unloadProximityMeters' assets/world src/ecs/services
rg -n 'HudComponent|HudSystem|textureEnabled|showValueText' src
```

Use these searches when you need to see how a component, factory key, or chunk entry is wired into the runtime.

## Common development flow

1. Edit a component, factory, or service.
2. Update its matching doc page under `docs/`.
3. Rebuild with `cmake --build build`.
4. Run `./build/duppy gameplay` and verify the content behaves as expected.
