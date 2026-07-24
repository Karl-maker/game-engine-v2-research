# MeshComponent

## Purpose

- MeshComponent
- Describes the geometric model an entity owns. The renderer/asset services turn
- the references into GPU buffers without hard-coding asset paths into systems.

## Use when

- Use this for any renderable model, including static props, skinned characters, and procedural meshes.

## Enums

- `MeshType`: `Static`, `Skinned`, `Procedural`

## Key fields

- `name` — A stable content or debug identifier.
- `materialIndex` — Texture or material binding.
- `indexOffset` — Spatial placement or relative offset.
- `indexCount` — Field of type `std::uint32_t` used by systems that consume this component.
- `min` — Field of type `math::Vec3` used by systems that consume this component.
- `max` — Field of type `math::Vec3` used by systems that consume this component.
- `enabled` — Enable or disable this part of the component.
- `meshId` — A stable content or debug identifier.
- `meshData` — Field of type `render::AssetRef` used by systems that consume this component.
- `meshType` — Field of type `MeshType` used by systems that consume this component.
- `subMeshes` — Field of type `std::vector<SubMesh>` used by systems that consume this component.
- `bounds` — Field of type `Bounds` used by systems that consume this component.
- `pivot` — Field of type `math::Vec3` used by systems that consume this component.
- `scale` — Size, reach, or distance tuning.
- `visible` — Field of type `bool` used by systems that consume this component.
- `castShadows` — Field of type `bool` used by systems that consume this component.
- `receiveShadows` — Field of type `bool` used by systems that consume this component.
- `tags` — Field of type `std::vector<std::string>` used by systems that consume this component.
- `skeletonId` — A stable content or debug identifier.
- `uvMappingTextureId` — Texture or material binding.

## Example

```cpp
auto& mesh = registry.emplace<ecs::MeshComponent>(entity);
mesh.enabled = true;
mesh.meshData.key = "assets/models/business-man/scene.gltf";
mesh.meshType = ecs::MeshComponent::MeshType::Skinned;
mesh.castShadows = true;
mesh.receiveShadows = true;
```

## Notes

- Pair this with `ShaderComponent` when you need a custom material or shader path.
- Use `skeletonId` and `uvMappingTextureId` when the asset needs animation skinning or a specific UV atlas.
