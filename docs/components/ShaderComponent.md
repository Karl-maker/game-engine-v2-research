# ShaderComponent

## Purpose

- ShaderComponent (descriptive only)
- Describes how a mesh should be rendered. A renderer interprets these values.
- This is intended to support high-quality graphics by allowing:
- - PBR-style textures (albedo/normal/orm/height/emissive)
- - Parameter blocks (colors, roughness/metallic, tiling, etc)
- - Render state controls (depth, blend, cull, shadows)

## Use when

- Use `ShaderComponent` when an entity needs a shader key, material parameters, texture bindings, or render-state overrides.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `shader` — Field of type `render::AssetRef` used by systems that consume this component.
- `renderMode` — Field of type `render::RenderMode` used by systems that consume this component.
- `cullMode` — Field of type `render::CullMode` used by systems that consume this component.
- `depthTest` — Field of type `render::DepthTest` used by systems that consume this component.
- `depthWrite` — Field of type `bool` used by systems that consume this component.
- `blendMode` — Field of type `render::BlendMode` used by systems that consume this component.
- `doubleSided` — Field of type `bool` used by systems that consume this component.
- `receiveShadows` — Field of type `bool` used by systems that consume this component.
- `castShadows` — Field of type `bool` used by systems that consume this component.
- `textures` — Texture or material binding.
- `parameters` — Field of type `std::vector<render::MaterialParameter>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::ShaderComponent>(entity);
component = ecs::ShaderComponent{};
```

## Notes

- Use `textures` for albedo/normal/roughness/emissive-style inputs and keep parameter names stable across materials.
- This component is the main place to describe how a mesh should look without hard-coding that look into systems.
