# FactoryKeyService

## Purpose

- Registers the built-in string keys that chunk JSON uses to spawn entities.

## Use when

- Use this when you add a new `IEntityFactory` implementation and want chunk data to resolve it by name.

## How it fits

- `ChunkStreamingService` reads chunk JSON.
- `FileChunkSource` or another `IChunkSource` returns the chunk definition.
- `EntityFactoryRegistry` looks up the factory key.
- `FactoryKeyService` registers the built-in mapping into the registry.

## Adding a new factory key

1. Create the factory class and its config struct.
2. Decide the JSON contract you want chunk authors to write.
3. Register the factory in `FactoryKeyService.cpp`.
4. Add an example chunk entry to the world content.
5. Document the key in the matching factory and chunk docs.

## What makes a good key

- Short and lower-case.
- Stable over time.
- Obvious to someone reading chunk JSON for the first time.

## Example

```cpp
ecs::services::EntityFactoryRegistry factories;
ecs::services::registerFactoriesFromEcsFactoriesDir(factories);
```

## Notes

- Keep keys short, stable, and lower-case so chunk files stay easy to read and diff.
- If a key disappears or changes, existing chunk content will fail to spawn that entity.
