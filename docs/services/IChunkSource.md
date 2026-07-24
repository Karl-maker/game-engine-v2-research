# IChunkSource

## Purpose

- IChunkSource provides reusable engine behavior.

## Use when

- Pluggable backend interface for chunk definitions.

## Example

```cpp
class DbChunkSource final : public ecs::services::IChunkSource {
 public:
  std::optional<ecs::services::ChunkDefinition> loadChunk(const ecs::services::ChunkCoord& coord) override;
};
```

## Notes

- Implement this if chunks should come from files, a database, remote storage, or generated content.
