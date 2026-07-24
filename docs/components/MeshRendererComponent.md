# MeshRendererComponent

## What it is

- MeshRendererComponent (descriptive only)
- Lightweight rendering hints for meshes (wireframe/opaque/etc).
- A renderer interprets these values.

## When to use it

- Use this when you want to separate draw behavior from the mesh asset reference itself.

## How it fits

- Treat `MeshRendererComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Keep it lightweight and let the renderer combine it with `MeshComponent` and `ShaderComponent`.

## Enums

- `RenderMode`: `Opaque`, `Transparent`, `Wireframe`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `renderMode` — Enum or bitmask value that changes system behavior.
- `hasMesh` — Stable reference used by content, loaders, or rendering systems.
- `meshId` — Stable reference used by content, loaders, or rendering systems.
- `hasMaterial` — Stable reference used by content, loaders, or rendering systems.
- `materialId` — Stable reference used by content, loaders, or rendering systems.
- `visibleLayers` — Enum or bitmask value that changes system behavior.

## Example

```cpp
auto& component = registry.emplace<ecs::MeshRendererComponent>(entity);
component = ecs::MeshRendererComponent{};
```

## Common pairings

- Pair `MeshRendererComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `MeshRendererComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
