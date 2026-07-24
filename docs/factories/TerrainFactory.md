# TerrainFactory

## Purpose

- TerrainFactory creates entities from data-driven config.

## Use when

- Creates terrain renderers and optional terrain colliders from JSON data.

## Config fields

- `name` — A stable content or debug identifier.
- `position` — Spatial placement or relative offset.
- `gridWidth` — Field of type `int` used by systems that consume this component.
- `gridHeight` — Field of type `int` used by systems that consume this component.
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
- `tessEnableDistance` — Enable or disable this part of the component.
- `tessDisableDistance` — Size, reach, or distance tuning.
- `viewDotBias` — Field of type `float` used by systems that consume this component.
- `colliderEnabled` — Enable or disable this part of the component.
- `colliderThicknessMeters` — Field of type `float` used by systems that consume this component.
- `shaderKey` — A stable content or debug identifier.
- `depthWrite` — Field of type `bool` used by systems that consume this component.

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

## Notes

- Use this when terrain should be data-driven and chunked like the rest of the world content.
