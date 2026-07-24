# FileChunkSource

## What it is

- FileChunkSource
- Loads chunk definitions from a JSON file.

## When to use it

- Use this as the default file-backed implementation while you are authoring or testing chunk content.

## Lifecycle

- `reload()` reparses the JSON file and rebuilds the in-memory chunk table.
- `loadChunk(coord)` looks up the parsed chunk definition and returns it if present.
- The source keeps the file path and parsed chunk map so the streaming service only deals with the `IChunkSource` contract.

## What to watch for

- Make sure the file uses the same chunk coordinate convention as the streamer.
- Keep the file readable so authors can add content without editing code.
- If the file changes on disk, call `reload()` before expecting new chunk content.

## Example

```cpp
ecs::services::FileChunkSource source("assets/world/chunks_demo.json");
source.reload();
auto chunk = source.loadChunk({0, 0});
```

## How it connects

- Integrate `FileChunkSource` where that responsibility belongs in the engine boundary.
- Swap it out later with a different `IChunkSource` implementation if you need database or network-backed chunks.

## Notes

- Keep the file format simple so future sources can match the same contract.
