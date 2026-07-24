# MeshAssetService

## What it is

- MeshAssetService
- Async CPU loader for mesh assets. Loaders are selected by file extension so
- OBJ/FBX/etc can be plugged in without changing ECS components or renderer code.

## When to use it

- Use this whenever a mesh key or path needs to resolve into GPU-ready geometry.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
assets::MeshAssetService meshes;
meshes.start();
meshes.request("assets/models/business-man/scene.gltf");
meshes.flushUploads();
```

## How it connects

- Works with `IMeshAssetLoader` implementations selected by file extension.
- Supplies mesh data to the renderer after the async worker finishes decoding.

## Notes

- Register a loader per extension so new file formats can be added without changing gameplay code.
