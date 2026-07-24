# AssetFactory

## What it is

- AssetFactory turns chunk JSON into live ECS entities.

## When to use it

- Use this for props, markers, or scene content that should be chunk-spawned but does not need custom gameplay logic yet.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- The config is currently minimal in the header, so the JSON contract is mostly defined in the factory implementation.

## Example JSON

```json
{
  "factory": "asset",
  "config": {
    "name": "prop_statue"
  }
}
```

## Adding a new field

- Add the setting to `AssetConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- This is a good place for authoring content that may later grow into a richer factory-specific type.
