# RenderSettingsComponent

## What it is

- RenderSettingsComponent (demo)
- Global-ish render switches for the demo renderer.

## When to use it

- Use this for scene-level debug controls or render-quality switches.

## How it fits

- Treat `RenderSettingsComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Treat this as the top-level visual settings bucket for the demo scene.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `shadowsEnabled` — Field of type `bool` consumed by systems that read this component.
- `shadowQuality` — Enum or bitmask value that changes system behavior.
- `shadowStrength` — Numeric tuning used by gameplay or rendering systems.
- `shadowUseTessellation` — Field of type `bool` consumed by systems that read this component.
- `showRays` — Field of type `bool` consumed by systems that read this component.
- `showCollisionBoxes` — Field of type `bool` consumed by systems that read this component.
- `showCombatBoxes` — Field of type `bool` consumed by systems that read this component.
- `showSkeletonBones` — Field of type `bool` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::RenderSettingsComponent>(entity);
component = ecs::RenderSettingsComponent{};
```

## Common pairings

- Pair `RenderSettingsComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `RenderSettingsComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
