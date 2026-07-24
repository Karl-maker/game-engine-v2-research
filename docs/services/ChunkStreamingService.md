# ChunkStreamingService

## Purpose

- ChunkStreamingService provides reusable engine behavior.

## Use when

- Streams chunk-defined entities around an anchor entity based on proximity and chunk coordinates.

## Example

```cpp
ecs::services::ChunkStreamingConfig cfg;
cfg.chunkSizeMeters = 32.0f;
cfg.loadProximityMeters = 18.0f;
cfg.unloadProximityMeters = 30.0f;

ecs::services::ChunkStreamingService streamer(cfg);
streamer.tick(registry, player, source, factories);
```

## Notes

- Use this with a swappable chunk source so file-backed chunks can later be replaced with database-backed chunks.
