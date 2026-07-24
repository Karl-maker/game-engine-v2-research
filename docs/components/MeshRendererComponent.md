# MeshRendererComponent

## Purpose

- MeshRendererComponent (descriptive only)
- Lightweight rendering hints for meshes (wireframe/opaque/etc).
- A renderer interprets these values.

## Use when

- Use this for render-only flags that should stay separate from the asset reference itself.

## Enums

- `RenderMode`: `Opaque`, `Transparent`, `Wireframe`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `renderMode` — Field of type `RenderMode` used by systems that consume this component.
- `hasMesh` — Field of type `bool` used by systems that consume this component.
- `meshId` — A stable content or debug identifier.
- `hasMaterial` — Texture or material binding.
- `materialId` — Texture or material binding.
- `visibleLayers` — Layer or filtering control.

## Example

```cpp
auto& renderer = registry.emplace<ecs::MeshRendererComponent>(entity);
renderer.enabled = true;
renderer.visible = true;
renderer.castShadows = true;
```

## Notes

- Keep this component focused on render behavior instead of duplicating mesh asset data.
