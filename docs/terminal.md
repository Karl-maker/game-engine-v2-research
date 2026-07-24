# Terminal Commands

Author: Karl-Johan Bailey

Use these commands while iterating on the game locally.

## Build

```bash
cmake -S . -B build
cmake --build build
```

What this does:

- configures the project into `build/`
- compiles the game binary
- reuses the same build directory across edits for faster rebuilds

## Run gameplay

```bash
./build/duppy gameplay
```

Use this when you want the normal gameplay scene with the current ECS and renderer pipeline.

## Run with fullscreen and high refresh targets

```bash
./build/duppy gameplay --fullscreen --refresh-rate 144 --no-vsync
```

Use this when your display supports higher refresh rates and you want to verify frame pacing without the default vsync ceiling.

## Debug modes and flags

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
