# MeshComponent

## What it is

- MeshComponent
- Describes the geometric model an entity owns. The renderer/asset services turn
- the references into GPU buffers without hard-coding asset paths into systems.

## When to use it

- Use this for any model or mesh that the renderer should draw.

## How it fits

- Treat `MeshComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set `meshData.key` to the model path or asset key.
- Choose the mesh type based on whether the asset is static, skinned, or procedural.
- Fill `skeletonId` when the mesh needs bone skinning.

## Enums

- `MeshType`: `Static`, `Skinned`, `Procedural`

## Field guide

- `name` — Stable reference used by content, loaders, or rendering systems.
- `materialIndex` — Stable reference used by content, loaders, or rendering systems.
- `indexOffset` — Spatial placement or directional tuning.
- `indexCount` — Count or budget control for work done by the system.
- `min` — Field of type `math::Vec3` consumed by systems that read this component.
- `max` — Field of type `math::Vec3` consumed by systems that read this component.
- `enabled` — Master on/off switch or similar behavior flag.
- `meshId` — Stable reference used by content, loaders, or rendering systems.
- `meshData` — Stable reference used by content, loaders, or rendering systems.
- `meshType` — Stable reference used by content, loaders, or rendering systems.
- `subMeshes` — Stable reference used by content, loaders, or rendering systems.
- `bounds` — Field of type `Bounds` consumed by systems that read this component.
- `pivot` — Field of type `math::Vec3` consumed by systems that read this component.
- `scale` — Size, reach, or distance tuning.
- `visible` — Master on/off switch or similar behavior flag.
- `castShadows` — Master on/off switch or similar behavior flag.
- `receiveShadows` — Master on/off switch or similar behavior flag.
- `tags` — Field of type `std::vector<std::string>` consumed by systems that read this component.
- `skeletonId` — Stable reference used by content, loaders, or rendering systems.
- `uvMappingTextureId` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::MeshComponent>(entity);
component = ecs::MeshComponent{};
```

## Common pairings

- Use `ShaderComponent` alongside this when the mesh needs custom material behavior.

## Notes

- Keep `MeshComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
