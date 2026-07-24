# SkyComponent

## What it is

- SkyComponent (descriptive only)
- Describes the sky covering the world (clouds, sun disc visuals, etc).
- This component does NOT represent a light; it is purely visual description.
- A rendering/sky system can interpret this component and configure the sky shader/material.

## When to use it

- Use this to drive the visual sky for a scene or level.

## How it fits

- Treat `SkyComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Decide whether the sky is procedural or skybox-based.
- Pick a sky type preset, then tune clouds and stars around it.
- Link directional light or fog entities if you want the sky to drive the rest of the scene.

## Enums

- `Mode`: `Procedural`, `Skybox`, `// cubemap/texture-based`
- `SkyType`: `Day`, `Sunset`, `Night`, `Overcast`, `Storm`
- `CloudType`: `None`, `Wispy`, `Scattered`, `Broken`, `Overcast`, `Storm`
- `Quality`: `Low`, `Medium`, `High`, `Ultra`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `mode` — Enum or bitmask value that changes system behavior.
- `skyType` — Enum or bitmask value that changes system behavior.
- `cloudType` — Enum or bitmask value that changes system behavior.
- `quality` — Enum or bitmask value that changes system behavior.
- `useSkyTypePreset` — Enum or bitmask value that changes system behavior.
- `linkedDirectionalLightEntity` — Master on/off switch or similar behavior flag.
- `linkedFogVolumeEntity` — Master on/off switch or similar behavior flag.
- `material` — Stable reference used by content, loaders, or rendering systems.
- `horizonColor` — Color, tint, or display styling.
- `zenithColor` — Color, tint, or display styling.
- `sunEnabled` — Master on/off switch or similar behavior flag.
- `sunDirection` — Spatial placement or directional tuning.
- `sunTint` — Color, tint, or display styling.
- `sunDiscIntensity` — Color, tint, or display styling.
- `sunDiscSize` — Size, reach, or distance tuning.
- `cloudsEnabled` — Master on/off switch or similar behavior flag.
- `cloudCoverage` — Numeric tuning used by gameplay or rendering systems.
- `cloudDensity` — Numeric tuning used by gameplay or rendering systems.
- `cloudSpeed` — Numeric tuning used by gameplay or rendering systems.
- `cloudWindDirection` — Spatial placement or directional tuning.
- `cloudTimeScale` — Size, reach, or distance tuning.
- `cloudTurbulence` — Field of type `float` consumed by systems that read this component.
- `cloudScale` — Size, reach, or distance tuning.
- `cloudLightAbsorption` — Field of type `float` consumed by systems that read this component.
- `cloudHeightMeters` — Size, reach, or distance tuning.
- `starsEnabled` — Master on/off switch or similar behavior flag.
- `starsIntensity` — Numeric tuning used by gameplay or rendering systems.
- `starsDensity` — Numeric tuning used by gameplay or rendering systems.
- `starsSize` — Size, reach, or distance tuning.
- `starsTwinkleStrength` — Numeric tuning used by gameplay or rendering systems.
- `starsTwinkleSpeed` — Numeric tuning used by gameplay or rendering systems.
- `starsSeed` — Field of type `std::uint32_t` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SkyComponent>(entity);
component = ecs::SkyComponent{};
```

## Common pairings

- Keep cloud and star quality aligned with your target hardware.

## Notes

- Keep `SkyComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
