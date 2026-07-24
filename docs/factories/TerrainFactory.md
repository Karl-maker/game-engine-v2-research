# TerrainFactory

## What it is

- TerrainFactory
- Creates a terrain renderable + optional terrain collider from data.

## When to use it

- Use this when terrain should come from chunk data or a world config file.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- `name` — Stable reference used by content, loaders, or rendering systems.
- `position` — Spatial placement or directional tuning.
- `gridWidth` — Stable reference used by content, loaders, or rendering systems.
- `gridHeight` — Stable reference used by content, loaders, or rendering systems.
- `cellSizeMeters` — Size, reach, or distance tuning.
- `heightScaleMeters` — Size, reach, or distance tuning.
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
- `colliderEnabled` — Master on/off switch or similar behavior flag.
- `colliderThicknessMeters` — Stable reference used by content, loaders, or rendering systems.
- `shaderKey` — Stable reference used by content, loaders, or rendering systems.
- `depthWrite` — Master on/off switch or similar behavior flag.

## Example JSON

```json
{
  "factory": "terrain",
  "config": {
    "name": "terrain_main",
    "gridWidth": 96,
    "gridHeight": 96,
    "cellSizeMeters": 1.0,
    "heightScaleMeters": 2.6,
    "noise": {
      "seed": 12345,
      "frequency": 0.03,
      "octaves": 2
    }
  }
}
```

## Adding a new field

- Add the setting to `TerrainConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- Match the terrain settings across adjacent chunks if you want the world to blend smoothly.
