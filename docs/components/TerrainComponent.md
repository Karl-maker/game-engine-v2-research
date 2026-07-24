# TerrainComponent

## Purpose

- TerrainComponent (descriptive only)
- Describes the terrain grid and how height should be generated.
- A terrain generation/rendering system interprets this component.

## Use when

- Use this for terrain grid generation, height sampling, tessellation thresholds, and terrain LOD tuning.

## Key fields

- `gridWidth` — Field of type `int` used by systems that consume this component.
- `gridHeight` — Field of type `int` used by systems that consume this component.
- `cellSizeMeters` — Size, reach, or distance tuning.
- `heightScaleMeters` — Size, reach, or distance tuning.
- `noiseSeed` — Field of type `std::uint32_t` used by systems that consume this component.
- `lodMaxRenderDistance` — Size, reach, or distance tuning.
- `lodStep1Distance` — Size, reach, or distance tuning.
- `lodStep2Distance` — Size, reach, or distance tuning.
- `lodStep4Distance` — Size, reach, or distance tuning.
- `lodStep8Distance` — Size, reach, or distance tuning.
- `lodStep16Distance` — Size, reach, or distance tuning.
- `lodForceNearDistance` — Size, reach, or distance tuning.
- `tessLockDistance` — Size, reach, or distance tuning.
- `tessEnableDistance` — Enable or disable this part of the component.
- `tessDisableDistance` — Size, reach, or distance tuning.
- `viewDotBias` — Field of type `float` used by systems that consume this component.
- `noise` — Field of type `terrain::NoiseConfig` used by systems that consume this component.

## Example

```cpp
auto& terrain = registry.emplace<ecs::TerrainComponent>(entity);
terrain.gridWidth = 512;
terrain.gridHeight = 512;
terrain.cellSizeMeters = 1.0f;
terrain.heightScaleMeters = 150.0f;
```

## Notes

- Keep `noiseSeed` stable when you want adjacent terrain chunks to merge seamlessly.
- Tune the LOD distances against your camera refresh and desired view radius.
