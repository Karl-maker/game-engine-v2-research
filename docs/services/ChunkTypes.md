# ChunkTypes

## What it is

- ChunkTypes
- Minimal types for chunk streaming + spawning.

## When to use it

- Use `ChunkTypes` when you want chunk streaming without hard-coding it into gameplay classes.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
ChunkTypes service{};
```

## How it connects

- Integrate `ChunkTypes` where that responsibility belongs in the engine boundary.

## Notes

- Prefer services when the work is reusable, cross-cutting, or expensive to duplicate.
- Keep shared state inside the service so callers do not need to coordinate the details themselves.
