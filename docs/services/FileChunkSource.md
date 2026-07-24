# FileChunkSource

## Purpose

- FileChunkSource provides reusable engine behavior.

## Use when

- Loads chunk definitions from a JSON file and serves them by chunk coordinate.

## Example

```cpp
ecs::services::FileChunkSource source("assets/world/chunks_demo.json");
source.reload();
auto chunk = source.loadChunk({0, 0});
```

## Notes

- This is the current default source, but the interface is intentionally swappable for other backends.
