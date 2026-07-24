# ChunkTypes

## Purpose

- Defines the small value types used by chunk streaming and factory spawning.

## Use when

- Use this page when you need to understand how chunk coordinates and chunk spawn payloads are represented in memory.

## Types

- `ChunkCoord` — chunk position on the X/Z plane.
- `ChunkCoordHash` — hash function so chunk coordinates can be used in unordered maps.
- `ChunkEntitySpawn` — one spawn entry with a `factory` key and JSON `config`.
- `ChunkDefinition` — a full chunk, including its coordinate and all of its spawn entries.

## Coordinate rules

- `coord.x` maps to world +X.
- `coord.y` maps to world +Z.
- `coord = {0, 0}` is the origin chunk.
- Chunks with larger coordinates load as the player approaches them in world space.

## Example

```cpp
ecs::services::ChunkCoord coord{0, 1};
ecs::services::ChunkDefinition chunk;
chunk.coord = coord;
chunk.entities.push_back({
    .factory = "character",
    .config = data::JsonValue::Object{}
});
```

## Notes

- These are data carriers, not systems.
- Keep them simple so chunk loading can stay file-backed today and database-backed later without changing the shape of the runtime API.
