# SpatialHashGridService

## Purpose

- SpatialHashGridService provides reusable engine behavior.

## Use when

- Uniform spatial hash broadphase that keeps collision candidate generation near linear time in local density.

## Example

```cpp
ecs::services::SpatialHashGridService grid(4.0f);
grid.insert({player, {{-1,0,-1}, {1,2,1}}});
auto pairs = grid.candidatePairs();
```

## Notes

- Use this for collision and ray queries before expensive narrow-phase work.
