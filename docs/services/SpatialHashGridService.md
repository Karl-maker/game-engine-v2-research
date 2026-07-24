# SpatialHashGridService

## What it is

- Uniform spatial hash broadphase for collision systems.
- It keeps pair generation near O(n + local pairs) instead of O(n^2).

## When to use it

- Use this for collision candidates, local queries, or ray lookups before narrow-phase work.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
ecs::services::SpatialHashGridService grid(4.0f);
grid.insert({player, {{-1, 0, -1}, {1, 2, 1}}});
auto pairs = grid.candidatePairs();
```

## How it connects

- Integrate `SpatialHashGridService` where that responsibility belongs in the engine boundary.

## Notes

- This is the right place to keep expensive `n^2` checks out of the main collision loop.
