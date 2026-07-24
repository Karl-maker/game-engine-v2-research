# CharacterFactory

## What it is

- CharacterFactory turns chunk JSON into live ECS entities.

## When to use it

- Use this for player characters, NPCs, and humanoid actors that share the same control and animation pipeline.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- `meshReference` — Stable reference used by content, loaders, or rendering systems.
- `leftHandKey` — Stable reference used by content, loaders, or rendering systems.
- `rightHandKey` — Stable reference used by content, loaders, or rendering systems.
- `headKey` — Stable reference used by content, loaders, or rendering systems.

## Example JSON

```json
{
  "factory": "character",
  "config": {
    "name": "hero",
    "meshReference": "assets/models/business-man/scene.gltf",
    "leftHandKey": "left_hand",
    "rightHandKey": "right_hand",
    "headKey": "head",
    "positionLocal": [0.0, 0.0, 0.0],
    "rotationDeg": [0.0, 180.0, 0.0],
    "mass": 22.0
  }
}
```

## Adding a new field

- Add the setting to `CharacterConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- This is the primary entry point for skinned actor spawning in chunk files.
