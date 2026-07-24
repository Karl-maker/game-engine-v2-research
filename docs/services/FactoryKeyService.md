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

## Example

```cpp
ecs::services::EntityFactoryRegistry factories;
ecs::services::registerFactoriesFromEcsFactoriesDir(factories);
```

## Adding a new key

1. Add a factory class in `src/ecs/factories/`.
2. Give it a config struct that matches the JSON you want to accept.
3. Register it in `FactoryKeyService.cpp`.
4. Add the new source file to `CMakeLists.txt`.
5. Reference the new key from chunk JSON with `"factory": "your_key"`.

## Notes

- Keep keys short, stable, and lower-case so chunk files stay easy to read and diff.
- If a key disappears or changes, existing chunk content will fail to spawn that entity.
