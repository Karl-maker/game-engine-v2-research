# BuildingFactory

## What it is

- BuildingFactory turns chunk JSON into live ECS entities.

## When to use it

- Use this when a building should spawn from chunk data instead of being placed manually in code.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- The config is currently minimal in the header, so the JSON contract is mostly defined in the factory implementation.

## Example JSON

```json
{
  "factory": "building",
  "config": {
    "name": "hut_01"
  }
}
```

## Adding a new field

- Add the setting to `BuildingConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- Keep this key stable so level content can reference buildings without code changes.
