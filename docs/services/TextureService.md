# TextureService

## Purpose

- TextureService provides reusable engine behavior.

## Use when

- Asynchronously loads image files, decodes them, and uploads textures on the OpenGL thread.

## Example

```cpp
graphics::TextureService textures;
textures.start();
textures.requestTexture("assets/textures/ui/life_bar.png", true);
textures.flushUploads();
```

## Notes

- Call `flushUploads()` from the render thread before you draw textured HUD or mesh materials.
