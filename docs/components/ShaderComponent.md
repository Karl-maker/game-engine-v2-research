# ShaderComponent

## What it is

- ShaderComponent (descriptive only)
- Describes how a mesh should be rendered. A renderer interprets these values.
- This is intended to support high-quality graphics by allowing:
- - PBR-style textures (albedo/normal/orm/height/emissive)
- - Parameter blocks (colors, roughness/metallic, tiling, etc)
- - Render state controls (depth, blend, cull, shadows)

## When to use it

- Use this when a renderable needs a specific shader or a custom material setup.

## How it fits

- Treat `ShaderComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set the shader key first so the renderer can build the right program.
- Bind textures and material parameters after the shader path is known.
- Use render-state overrides only when the default mesh pipeline is not enough.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `shader` — Stable reference used by content, loaders, or rendering systems.
- `renderMode` — Enum or bitmask value that changes system behavior.
- `cullMode` — Enum or bitmask value that changes system behavior.
- `depthTest` — Field of type `render::DepthTest` consumed by systems that read this component.
- `depthWrite` — Master on/off switch or similar behavior flag.
- `blendMode` — Enum or bitmask value that changes system behavior.
- `doubleSided` — Stable reference used by content, loaders, or rendering systems.
- `receiveShadows` — Master on/off switch or similar behavior flag.
- `castShadows` — Master on/off switch or similar behavior flag.
- `textures` — Stable reference used by content, loaders, or rendering systems.
- `parameters` — Field of type `std::vector<render::MaterialParameter>` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::ShaderComponent>(entity);
component = ecs::ShaderComponent{};
```

## Common pairings

- Keep texture keys stable because content and materials often refer to them by path.

## Notes

- Keep `ShaderComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
