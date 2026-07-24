# IChunkSource

## What it is

- IChunkSource
- Pluggable data provider for chunk definitions.

## When to use it

- Implement this when chunks should come from files, a database, or generated content.

## Lifecycle

- A streaming service asks for a chunk coordinate.
- The chunk source looks up that coordinate in its backend.
- If the chunk exists, the source returns its definition; otherwise it returns nothing.

## What a good implementation does

- Keeps the public contract small: coordinate in, optional chunk out.
- Hides the storage backend, whether that backend is a JSON file, a database, or generated content.
- Uses the same coordinate system as the streamer so chunk lookup stays predictable.

## Example

```cpp
class DbChunkSource final : public ecs::services::IChunkSource {
 public:
  std::optional<ecs::services::ChunkDefinition> loadChunk(const ecs::services::ChunkCoord& coord) override;
};
```

## How it connects

- Integrate `IChunkSource` where that responsibility belongs in the engine boundary.
- This is the seam you replace when you move from file-backed content to database-backed content.

## Notes

- Keep the return shape identical so the streaming service does not care where the chunk came from.
