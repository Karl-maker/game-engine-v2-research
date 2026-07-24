# TextureService

## What it is

- TextureService (demo-focused)
- Asynchronously loads image files, decodes them, and uploads to OpenGL textures
- on the render thread.

## When to use it

- Use this when meshes or HUD widgets need texture keys resolved into live GL textures.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
graphics::TextureService textures;
textures.start();
textures.requestTexture("assets/textures/ui/life_bar.png", true);
textures.flushUploads();
```

## How it connects

- Resolves image paths into live GPU textures.
- Must flush uploads on the render thread before drawing textured content.

## Notes

- Call `flushUploads()` on the GL thread before drawing textured content.
