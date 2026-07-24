# VfxComponent

## Purpose

- VfxComponent (descriptive only)
- Describes a visual effect emitter such as fire, electricity, sparks, smoke, or steam.
- Rendering systems interpret these settings and apply LOD / quality-based budgets.

## Use when

- Use this for fire, electricity, sparks, smoke, steam, and other emitter-style visual effects.

## Enums

- `Type`: `Fire`, `Electricity`, `Sparks`, `Smoke`, `Steam`
- `Quality`: `Low`, `Medium`, `High`, `Ultra`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `type` — Field of type `Type` used by systems that consume this component.
- `quality` — Level-of-detail or quality control.
- `autoQuality` — Level-of-detail or quality control.
- `maxRenderDistance` — Size, reach, or distance tuning.
- `lodNearDistance` — Size, reach, or distance tuning.
- `lodMidDistance` — Size, reach, or distance tuning.
- `lodFarDistance` — Size, reach, or distance tuning.
- `lodUltraDistance` — Size, reach, or distance tuning.
- `lodForceNearDistance` — Size, reach, or distance tuning.
- `viewDotBias` — Field of type `float` used by systems that consume this component.
- `intensity` — Field of type `float` used by systems that consume this component.
- `spawnRate` — Field of type `float` used by systems that consume this component.
- `burstInterval` — Color or tint control.
- `lifetimeSeconds` — Field of type `float` used by systems that consume this component.
- `sizeMeters` — Size, reach, or distance tuning.
- `sizeVariance` — Size, reach, or distance tuning.
- `speedMetersPerSecond` — Movement or force tuning.
- `speedVariance` — Movement or force tuning.
- `gravityScale` — Size, reach, or distance tuning.
- `drag` — Field of type `float` used by systems that consume this component.
- `flickerStrength` — Field of type `float` used by systems that consume this component.
- `flickerSpeed` — Movement or force tuning.
- `looping` — Field of type `bool` used by systems that consume this component.
- `castLight` — Field of type `bool` used by systems that consume this component.
- `seed` — Field of type `std::uint32_t` used by systems that consume this component.
- `heightMeters` — Field of type `float` used by systems that consume this component.
- `upwardBias` — Field of type `float` used by systems that consume this component.
- `spreadRadiusMeters` — Size, reach, or distance tuning.
- `heatHazeStrength` — Field of type `float` used by systems that consume this component.
- `chargeLengthMeters` — Field of type `float` used by systems that consume this component.
- `arcJitter` — Field of type `float` used by systems that consume this component.
- `branchCount` — Field of type `int` used by systems that consume this component.
- `segmentCount` — Field of type `int` used by systems that consume this component.
- `pulseSpeed` — Movement or force tuning.
- `sparkCount` — Field of type `int` used by systems that consume this component.
- `sparkSpreadDegrees` — Field of type `float` used by systems that consume this component.
- `sparkTrailLengthMeters` — Field of type `float` used by systems that consume this component.
- `sparkFadeSeconds` — Field of type `float` used by systems that consume this component.
- `primaryColor` — Color or tint control.
- `secondaryColor` — Color or tint control.

## Example

```cpp
auto& vfx = registry.emplace<ecs::VfxComponent>(entity);
vfx.type = ecs::VfxComponent::Type::Fire;
vfx.quality = ecs::VfxComponent::Quality::High;
vfx.spawnRate = 18.0f;
vfx.maxRenderDistance = 96.0f;
```

## Notes

- Use the LOD fields to reduce cost as the effect gets farther from the camera.
- If the effect should interact with gameplay, keep that in a separate component or system.
