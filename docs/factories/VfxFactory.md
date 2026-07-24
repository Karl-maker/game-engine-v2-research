# VfxFactory

## What it is

- VfxFactory
- Creates a visual-effect emitter entity from data.

## When to use it

- Use this when effects should be spawned through chunk data and respect runtime LOD/quality rules.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- `name` — Stable reference used by content, loaders, or rendering systems.
- `position` — Spatial placement or directional tuning.
- `rotationDeg` — Orientation or angular tuning.
- `scale` — Size, reach, or distance tuning.
- `component` — Field of type `ecs::VfxComponent` consumed by systems that read this component.

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

## Adding a new field

- Add the setting to `VfxConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- Keep effect quality and distance budgets tuned so VFX does not dominate frame time.
