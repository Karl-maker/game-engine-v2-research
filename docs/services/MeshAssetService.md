# MeshAssetService

## Purpose

- MeshAssetService provides reusable engine behavior.

## Use when

- Loads mesh assets asynchronously on the CPU and uploads them through the render pipeline.

## Example

```cpp
assets::MeshAssetService meshes;
meshes.start();
meshes.request("assets/models/business-man/scene.gltf");
meshes.flushUploads();
```

## Notes

- Register a loader per extension so new asset formats can be added without changing gameplay code.
