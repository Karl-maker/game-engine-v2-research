# GrassPatchComponent

## What it is

- GrassPatchComponent (descriptive only)
- Describes GPU-instanced vegetation (primarily grass) for another system to generate/render.
- Design goals (demo-friendly, scalable):
- - No ECS entity per blade/clump; a renderer/system builds instance buffers.
- - Multiple layers/species to avoid obvious repetition.
- - Density/slope/altitude/noise rules for natural variation.
- - Wind + interaction are handled in shaders (driven by shared uniforms).

## When to use it

- Use this for dense grass areas that should render as a repeating patch instead of individual entities.

## How it fits

- Treat `GrassPatchComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Keep the patch size and density aligned with terrain LOD so vegetation fades coherently.

## Field guide

- `species` — Field of type `std::string` consumed by systems that read this component.
- `description` — Field of type `std::string` consumed by systems that read this component.
- `density` — Numeric tuning used by gameplay or rendering systems.
- `minScale` — Size, reach, or distance tuning.
- `maxScale` — Size, reach, or distance tuning.
- `bladeSpacing` — Field of type `float` consumed by systems that read this component.
- `bendStrength` — Numeric tuning used by gameplay or rendering systems.
- `curveStrength` — Numeric tuning used by gameplay or rendering systems.
- `twistStrength` — Numeric tuning used by gameplay or rendering systems.
- `minSlopeDeg` — Field of type `float` consumed by systems that read this component.
- `maxSlopeDeg` — Field of type `float` consumed by systems that read this component.
- `minAltitude` — Field of type `float` consumed by systems that read this component.
- `maxAltitude` — Field of type `float` consumed by systems that read this component.
- `noiseScale` — Size, reach, or distance tuning.
- `noiseStrength` — Numeric tuning used by gameplay or rendering systems.
- `windStrength` — Numeric tuning used by gameplay or rendering systems.
- `maxDistance` — Size, reach, or distance tuning.
- `enabled` — Master on/off switch or similar behavior flag.
- `area` — Field of type `math::Vec3` consumed by systems that read this component.
- `densityMultiplier` — Numeric tuning used by gameplay or rendering systems.
- `seed` — Field of type `std::uint32_t` consumed by systems that read this component.
- `densityNoiseThreshold` — Numeric tuning used by gameplay or rendering systems.
- `densityNoiseContrast` — Numeric tuning used by gameplay or rendering systems.
- `densityNoiseStrength` — Numeric tuning used by gameplay or rendering systems.
- `islandNoiseOffset` — Spatial placement or directional tuning.
- `islandNoiseThreshold` — Field of type `float` consumed by systems that read this component.
- `islandNoiseSoftness` — Field of type `float` consumed by systems that read this component.
- `islandNoiseContrast` — Field of type `float` consumed by systems that read this component.
- `islandNoiseStrength` — Numeric tuning used by gameplay or rendering systems.
- `sourceTerrainEntity` — Reference to another entity or a linked runtime object.
- `interactionEnabled` — Field of type `bool` consumed by systems that read this component.
- `interactionRadiusMeters` — Size, reach, or distance tuning.
- `interactionStrength` — Numeric tuning used by gameplay or rendering systems.
- `castShadows` — Master on/off switch or similar behavior flag.
- `receiveShadows` — Master on/off switch or similar behavior flag.
- `lodBias` — Field of type `float` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::GrassPatchComponent>(entity);
component = ecs::GrassPatchComponent{};
```

## Common pairings

- Pair `GrassPatchComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `GrassPatchComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
