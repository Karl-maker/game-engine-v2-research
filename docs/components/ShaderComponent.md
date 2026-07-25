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

## Mesh material slots

For mesh shaders, the current renderer now understands these texture slot names:

- `albedo`, `baseColor`, `base_color`
- `normalgl`, `normal`
- `roughness`
- `metallic`
- `ao`, `ambient_occlusion`
- `specular`
- `emissive`
- `displacement`, `height`
- `metallicRoughness`, `metallic_roughness`
- `orm`

Useful mesh material parameters:

- `baseColor`
- `roughness`
- `metallic`
- `specularIntensity`
- `normalScale` or `normalStrength`
- `aoStrength`
- `emissiveColor`
- `emissiveStrength`
- `displacementStrength`

## UVs

- Mesh UVs are loaded from the asset automatically when the source file already contains them.
- The engine does not generate new UV unwraps during load.
- If a mesh has no UVs, texture refs can still be assigned, but mapping quality depends entirely on the source asset.

## Automatic glTF material import

- The mesh loader now auto-imports glTF `baseColorTexture`, `normalTexture`, `metallicRoughnessTexture`, `occlusionTexture`, and `emissiveTexture`.
- It also imports `baseColorFactor`, `roughnessFactor`, `metallicFactor`, `normalTexture.scale`, `occlusionTexture.strength`, and `emissiveFactor`.
- If the asset uses `KHR_materials_specular`, the loader also imports `specularFactor` and `specularTexture`.
- Explicit `ShaderComponent` texture refs or parameters still win over imported file material data.

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
component.shader.key = "graphics/shaders/model";
component.textures.push_back({"albedo", render::AssetRef{true, "assets/textures/stone/stone_color.jpg", 0}, true});
component.textures.push_back({"normalgl", render::AssetRef{true, "assets/textures/stone/stone_normalgl.jpg", 0}, false});
component.textures.push_back({"roughness", render::AssetRef{true, "assets/textures/stone/stone_roughness.jpg", 0}, false});
component.textures.push_back({"ao", render::AssetRef{true, "assets/textures/stone/stone_ambientocclusion.jpg", 0}, false});
component.parameters.push_back({"roughness", 0.85f});
component.parameters.push_back({"metallic", 0.0f});
component.parameters.push_back({"specularIntensity", 0.35f});
component.parameters.push_back({"normalScale", 1.25f});
```

## Common pairings

- Keep texture keys stable because content and materials often refer to them by path.
- Use `orm` or `metallicRoughness` when the source workflow already exports packed textures.

## Notes

- Keep `ShaderComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
