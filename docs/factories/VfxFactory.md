# VfxFactory

## Purpose

- VfxFactory creates entities from data-driven config.

## Use when

- Creates visual-effect emitters such as fire, electricity, sparks, smoke, and steam.

## Config fields

- `name` — A stable content or debug identifier.
- `position` — Spatial placement or relative offset.
- `rotationDeg` — Orientation or angle tuning.
- `scale` — Size, reach, or distance tuning.
- `component` — Field of type `ecs::VfxComponent` used by systems that consume this component.

## Example JSON

```json
{
  "factory": "vfx",
  "config": {
    "name": "campfire_vfx",
    "type": "Fire",
    "quality": "High",
    "positionLocal": [2.0, 0.0, 3.0],
    "spawnRate": 30.0,
    "maxRenderDistance": 96.0
  }
}
```

## Notes

- Use the effect quality and LOD settings to keep the scene responsive on low-end hardware.
