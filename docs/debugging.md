# Debugging And Profiling

Author: Karl-Johan Bailey

This page explains how to use the built-in debugging and frame-profiling tools.

## Quick start

Use the frame debugger without heavy debug geometry:

```bash
./build/duppy gameplay --debug-overlay
```

Use world-space debug drawing:

```bash
./build/duppy gameplay --debug-world
```

Use both together:

```bash
./build/duppy gameplay --debug
```

## What each flag does

- `--debug-overlay` turns on the frame debugger overlay and title-bar timing summary.
- `--debug-world` turns on rays, collision boxes, combat volumes, and skeleton debug lines.
- `--debug` turns on both.
- `--fps 144 --uncapped` helps when you want to compare capped and uncapped behavior.
- `--fullscreen --refresh-rate 144 --no-vsync` helps when you want to remove common 60 FPS display limits.

## Why the split matters

The overlay and the world debug drawing are not the same thing.

- The overlay mostly costs text rendering and string formatting.
- The world debug drawing adds real scene work because it pushes extra lines, boxes, and skeleton guides through the renderer.

If you are asking “what is slow in the real game?”, start with `--debug-overlay`.

## Reading the frame debugger

The overlay shows:

- `fps`, `dt_ms`, `cpu_ms`, `render_ms`, and `gpu_ms`
- the hottest CPU systems from the gameplay tick
- the previous frame’s hottest render passes
- scene counts such as terrains, meshes, grasses, VFX, HUD widgets, rays, and debug lines
- whether world debug drawing is currently on or off

Interpretation tips:

- High `cpu_ms` with lower `render_ms` usually means simulation, asset processing, collision, or snapshot building is the main bottleneck.
- High `render_ms` with lower `cpu_ms` usually means the renderer, GPU work, post effects, or debug drawing is the main bottleneck.
- High `gpu_ms` means the GPU is actually spending time on the submitted frame, which usually points to terrain, grass, shadows, or post effects.
- High `render_ms` with much lower `gpu_ms` usually means CPU-side render setup is still a major part of the cost.
- If numbers spike only when `world_debug=on`, the debug view itself is part of the slowdown.

## Current likely hotspots in this project

The frame debugger is there so you can confirm this on your machine, but the current codebase has a few obvious expensive areas:

- Terrain rendering can be heavy because it supports LOD, tessellation decisions, multiple texture lookups, and optional shadow passes.
- Grass rendering can be expensive because large instanced patches still do a lot of per-frame visibility and draw work.
- VFX are generated on the CPU every frame before they are drawn, especially fire, sparks, and electricity emitters.
- Shadow rendering adds another major pass for terrain, meshes, and rocks.
- Depth of field and motion blur add post-processing passes on top of the main scene.
- Debug-world rendering adds extra lines and overlays that are useful, but not free.

## How optimized is it right now

The project is moderately optimized, not heavily optimized.

What is already good:

- chunk streaming exists, so the world does not need to be authored as one giant static scene
- terrain and VFX have LOD and distance-based culling controls
- broadphase collision uses a spatial hash service instead of a pure `n^2` check
- async mesh and texture loading are in place
- renderer uniform locations are cached instead of being looked up from OpenGL by name every frame
- GPU timer queries now expose approximate frame GPU cost in the overlay and title bar

What is still not highly optimized:

- the renderer still does a lot of CPU-side setup per pass
- VFX are rebuilt every frame on the CPU
- debug and profiling features were previously coupled together, which made measurement noisy
- GPU timing is frame-level only, not a per-pass GPU breakdown
- there is still no vendor-specific GPU profiler integration yet

## Recommended profiling workflow

1. Start with `./build/duppy gameplay --debug-overlay`.
2. Watch which CPU system becomes the hottest over several seconds.
3. Compare `render_ms` and `gpu_ms` to decide whether the bottleneck is mostly CPU-side render setup or actual GPU frame time.
4. Watch which render pass is hottest in the previous-frame render summary.
5. Toggle features mentally by scene content: grass, VFX, shadows, HUD, and post effects.
6. Only then use `--debug-world` if you need to inspect queries, collisions, or sockets.
7. If the game is still slow with overlay only, the slowdown is much more likely to be real scene cost than just the machine drawing debug helpers.

## Machine or code

Without external GPU tools, you should assume it can be both.

- A weaker CPU or GPU will make the expensive passes show up sooner.
- The current code also has real optimization headroom, especially in rendering and per-frame CPU setup.

The new frame debugger helps answer the practical version of that question:

- if `cpu_ms` is dominant, the code path on the CPU needs attention
- if `render_ms` is dominant and `gpu_ms` is also high, the GPU workload needs attention
- if `render_ms` is dominant but `gpu_ms` stays much lower, CPU-side render submission/setup needs attention
- if both are low but FPS is still capped, check fullscreen mode, refresh rate, vsync, and monitor settings
