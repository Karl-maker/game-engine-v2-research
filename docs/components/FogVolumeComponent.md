# FogVolumeComponent

## What it is

- FogVolumeComponent (descriptive)
- Defines a simple axis-aligned fog/mist volume for the renderer to apply.
- The graphics system collects these and the renderer applies fog in shaders.

## When to use it

- Use this for local weather, haze, smoke, or environmental visibility zones.

## How it fits

- Treat `FogVolumeComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Place the volume around the region that should receive fog and tune density against scene scale.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `sizeMeters` — Size, reach, or distance tuning.
- `color` — Color, tint, or display styling.
- `density` — Numeric tuning used by gameplay or rendering systems.
- `startDistance` — Size, reach, or distance tuning.
- `endDistance` — Size, reach, or distance tuning.
- `heightFalloff` — Size, reach, or distance tuning.
- `baseHeightOffset` — Spatial placement or directional tuning.

## Example

```cpp
auto& component = registry.emplace<ecs::FogVolumeComponent>(entity);
component = ecs::FogVolumeComponent{};
```

## Common pairings

- Pair `FogVolumeComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `FogVolumeComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
