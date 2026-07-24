# ChunkStreamingService

## What it is

- ChunkStreamingService
- Loads/unloads chunk-defined entities around an anchor entity based on world position.

## When to use it

- Use this when world content should appear as the player approaches chunk coordinates and disappear when too far away.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
ecs::services::ChunkStreamingConfig cfg;
cfg.chunkSizeMeters = 32.0f;
cfg.loadProximityMeters = 18.0f;
cfg.unloadProximityMeters = 30.0f;

ecs::services::ChunkStreamingService streamer(cfg);
streamer.tick(registry, player, source, factories);
```

## How it connects

- Works with `IChunkSource` for chunk data.
- Works with `EntityFactoryRegistry` for spawn keys.
- Usually follows a player or camera anchor entity.

## Streaming workflow

1. Read the anchor entity position.
2. Convert world position into chunk coordinates.
3. Ask the chunk source for nearby chunk definitions.
4. Spawn entities for chunks inside the load range.
5. Keep loaded chunk ids around until the unload range is exceeded.
6. Destroy chunk-owned entities when they leave the unload range.

## Practical tuning

- Keep `loadProximityMeters` lower than `unloadProximityMeters` so chunk edges do not thrash.
- Set `searchRadiusChunks` large enough to see a few chunks ahead of the player, but not so large that you scan the entire world every frame.
- Make `chunkSizeMeters` match your level design units so the chunk grid feels natural.

## Notes

- The anchor entity usually is the player or camera target.
- Keep the load and unload distances different so chunks do not churn at the threshold.
- Use a swappable `IChunkSource` so file-backed content can later come from a database or remote service.
