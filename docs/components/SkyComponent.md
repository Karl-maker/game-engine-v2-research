# SkyComponent

## Purpose

- SkyComponent (descriptive only)
- Describes the sky covering the world (clouds, sun disc visuals, etc).
- This component does NOT represent a light; it is purely visual description.
- A rendering/sky system can interpret this component and configure the sky shader/material.

## Use when

- Use this to describe the sky, clouds, sun disc, stars, and linked environment settings for a scene.

## Enums

- `Mode`: `Procedural`, `Skybox`, `// cubemap/texture-based`
- `SkyType`: `Day`, `Sunset`, `Night`, `Overcast`, `Storm`
- `CloudType`: `None`, `Wispy`, `Scattered`, `Broken`, `Overcast`, `Storm`
- `Quality`: `Low`, `Medium`, `High`, `Ultra`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `mode` — Field of type `Mode` used by systems that consume this component.
- `skyType` — Field of type `SkyType` used by systems that consume this component.
- `cloudType` — Field of type `CloudType` used by systems that consume this component.
- `quality` — Level-of-detail or quality control.
- `useSkyTypePreset` — Field of type `bool` used by systems that consume this component.
- `linkedDirectionalLightEntity` — Field of type `EntityId` used by systems that consume this component.
- `linkedFogVolumeEntity` — Field of type `EntityId` used by systems that consume this component.
- `material` — Texture or material binding.
- `horizonColor` — Color or tint control.
- `zenithColor` — Color or tint control.
- `sunEnabled` — Enable or disable this part of the component.
- `sunDirection` — Field of type `math::Vec3` used by systems that consume this component.
- `sunTint` — Color or tint control.
- `sunDiscIntensity` — Field of type `float` used by systems that consume this component.
- `sunDiscSize` — Size, reach, or distance tuning.
- `cloudsEnabled` — Enable or disable this part of the component.
- `cloudCoverage` — Field of type `float` used by systems that consume this component.
- `cloudDensity` — Field of type `float` used by systems that consume this component.
- `cloudSpeed` — Movement or force tuning.
- `cloudWindDirection` — Field of type `math::Vec2` used by systems that consume this component.
- `cloudTimeScale` — Size, reach, or distance tuning.
- `cloudTurbulence` — Field of type `float` used by systems that consume this component.
- `cloudScale` — Size, reach, or distance tuning.
- `cloudLightAbsorption` — Field of type `float` used by systems that consume this component.
- `cloudHeightMeters` — Field of type `float` used by systems that consume this component.
- `starsEnabled` — Enable or disable this part of the component.
- `starsIntensity` — Field of type `float` used by systems that consume this component.
- `starsDensity` — Field of type `float` used by systems that consume this component.
- `starsSize` — Size, reach, or distance tuning.
- `starsTwinkleStrength` — Field of type `float` used by systems that consume this component.
- `starsTwinkleSpeed` — Movement or force tuning.
- `starsSeed` — Field of type `std::uint32_t` used by systems that consume this component.

## Example

```cpp
auto& sky = registry.emplace<ecs::SkyComponent>(entity);
sky.mode = ecs::SkyComponent::Mode::Procedural;
sky.skyType = ecs::SkyComponent::SkyType::Day;
sky.cloudsEnabled = true;
sky.starsEnabled = false;
```

## Notes

- Use the linked light and fog entity ids when you want the sky preset to keep the rest of the scene coherent.
