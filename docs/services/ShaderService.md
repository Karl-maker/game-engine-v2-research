# ShaderService

## What it is

- ShaderService (OpenGL)
- Loads shader source from disk, expands `#include "..."`, and builds GL programs.
- Supports optional tessellation stages when `.tesc.glsl` and `.tese.glsl` exist.

## When to use it

- Use this when a render path needs a shader program by logical key instead of hard-coded GL setup.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
graphics::ShaderService shaders;
auto* program = shaders.getOrCreate("graphics/shaders/model");
```

## How it connects

- Turns shader keys into compiled OpenGL programs.
- Resolves include directives and caches the result for reuse across frames.

## Notes

- Keep shader keys stable because materials, render code, and content often refer to them directly.
