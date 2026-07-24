# EntityFactory

## What it is

- EntityFactory
- Simple "spawn an entity" factory used by chunk loading.

## When to use it

- Use this for spawn markers, simple props, or anything that only needs position and a bit of physics.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- `name` — Stable reference used by content, loaders, or rendering systems.
- `position` — Spatial placement or directional tuning.
- `rotationDeg` — Orientation or angular tuning.
- `scale` — Size, reach, or distance tuning.
- `hasRigidbody` — Stable reference used by content, loaders, or rendering systems.
- `mass` — Numeric tuning used by gameplay or rendering systems.
- `useGravity` — Master on/off switch or similar behavior flag.

## Example JSON

```json
{
  "factory": "entity",
  "config": {
    "name": "spawn_marker",
    "positionLocal": [0.0, 0.0, 0.0],
    "rotationDeg": [0.0, 90.0, 0.0],
    "scale": [1.0, 1.0, 1.0],
    "rigidbody": {
      "mass": 22.0,
      "useGravity": true
    }
  }
}
```

## Adding a new field

- Add the setting to `EntityConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- This is the lowest-friction way to get data into the world and then add behavior later.
