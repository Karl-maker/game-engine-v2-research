# GrassPatchComponent

## Purpose

- GrassPatchComponent (descriptive only)
- Describes GPU-instanced vegetation (primarily grass) for another system to generate/render.
- Design goals (demo-friendly, scalable):
- - No ECS entity per blade/clump; a renderer/system builds instance buffers.
- - Multiple layers/species to avoid obvious repetition.
- - Density/slope/altitude/noise rules for natural variation.
- - Wind + interaction are handled in shaders (driven by shared uniforms).

## Use when

- Use `GrassPatchComponent` when you need ECS data for grasspatchcomponent behavior.

## Key fields

- `species` — Field of type `std::string` used by systems that consume this component.
- `description` — Field of type `std::string` used by systems that consume this component.
- `density` — Field of type `float` used by systems that consume this component.
- `minScale` — Size, reach, or distance tuning.
- `maxScale` — Size, reach, or distance tuning.
- `bladeSpacing` — Field of type `float` used by systems that consume this component.
- `bendStrength` — Field of type `float` used by systems that consume this component.
- `curveStrength` — Field of type `float` used by systems that consume this component.
- `twistStrength` — Field of type `float` used by systems that consume this component.
- `minSlopeDeg` — Field of type `float` used by systems that consume this component.
- `maxSlopeDeg` — Field of type `float` used by systems that consume this component.
- `minAltitude` — Field of type `float` used by systems that consume this component.
- `maxAltitude` — Field of type `float` used by systems that consume this component.
- `noiseScale` — Size, reach, or distance tuning.
- `noiseStrength` — Field of type `float` used by systems that consume this component.
- `windStrength` — Field of type `float` used by systems that consume this component.
- `maxDistance` — Size, reach, or distance tuning.
- `enabled` — Enable or disable this part of the component.
- `area` — Field of type `math::Vec3` used by systems that consume this component.
- `densityMultiplier` — Field of type `float` used by systems that consume this component.
- `seed` — Field of type `std::uint32_t` used by systems that consume this component.
- `densityNoiseThreshold` — Field of type `float` used by systems that consume this component.
- `densityNoiseContrast` — Field of type `float` used by systems that consume this component.
- `densityNoiseStrength` — Field of type `float` used by systems that consume this component.
- `islandNoiseOffset` — Spatial placement or relative offset.
- `islandNoiseThreshold` — Field of type `float` used by systems that consume this component.
- `islandNoiseSoftness` — Field of type `float` used by systems that consume this component.
- `islandNoiseContrast` — Field of type `float` used by systems that consume this component.
- `islandNoiseStrength` — Field of type `float` used by systems that consume this component.
- `sourceTerrainEntity` — Reference to another entity or input source.
- `interactionEnabled` — Enable or disable this part of the component.
- `interactionRadiusMeters` — Size, reach, or distance tuning.
- `interactionStrength` — Field of type `float` used by systems that consume this component.
- `castShadows` — Field of type `bool` used by systems that consume this component.
- `receiveShadows` — Field of type `bool` used by systems that consume this component.
- `lodBias` — Level-of-detail or quality control.

## Example

```cpp
auto& component = registry.emplace<ecs::GrassPatchComponent>(entity);
component = ecs::GrassPatchComponent{};
```

## Notes

- Keep `GrassPatchComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
