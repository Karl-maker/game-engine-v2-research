# TerrainComponent

## What it is

- TerrainComponent (descriptive only)
- Describes the terrain grid and how height should be generated.
- A terrain generation/rendering system interprets this component.

## When to use it

- Use this for any terrain chunk or world surface that should render and collide as landscape.

## How it fits

- Treat `TerrainComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set the grid dimensions and cell size to define the terrain footprint.
- Keep the noise seed stable when neighboring chunks should merge cleanly.
- Tune LOD distances against your camera range and world scale.

## Common recipes

- **Chunked world terrain:** keep identical `noiseSeed` and matching noise settings across adjacent chunks.
- **Close-up terrain:** lower the cell size and keep the first LOD threshold near the camera for smoother detail.
- **Large vista terrain:** raise the LOD distances and keep `lodForceNearDistance` small enough that the near field still looks solid.

## Field guide

- `gridWidth` — Stable reference used by content, loaders, or rendering systems.
- `gridHeight` — Stable reference used by content, loaders, or rendering systems.
- `cellSizeMeters` — Size, reach, or distance tuning.
- `heightScaleMeters` — Size, reach, or distance tuning.
- `noiseSeed` — Field of type `std::uint32_t` consumed by systems that read this component.
- `lodMaxRenderDistance` — Size, reach, or distance tuning.
- `lodStep1Distance` — Size, reach, or distance tuning.
- `lodStep2Distance` — Size, reach, or distance tuning.
- `lodStep4Distance` — Size, reach, or distance tuning.
- `lodStep8Distance` — Size, reach, or distance tuning.
- `lodStep16Distance` — Size, reach, or distance tuning.
- `lodForceNearDistance` — Size, reach, or distance tuning.
- `tessLockDistance` — Size, reach, or distance tuning.
- `tessEnableDistance` — Size, reach, or distance tuning.
- `tessDisableDistance` — Size, reach, or distance tuning.
- `viewDotBias` — Field of type `float` consumed by systems that read this component.
- `noise` — Field of type `terrain::NoiseConfig` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::TerrainComponent>(entity);
component = ecs::TerrainComponent{};
```

## Common pairings

- Use the same terrain settings for adjacent chunks if you want smooth transitions.

## Notes

- Keep `TerrainComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
