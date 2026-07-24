# SteadyTimeSource

## What it is

- Monotonic time source (steady clock):
- - Suitable for delta time / frame timing (won't jump with system clock changes).
- - Returns seconds since construction.

## When to use it

- Use this when you need stable timing that should not jump if the system clock changes.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
core::SteadyTimeSource timeSource;
auto seconds = timeSource.nowSeconds();
```

## How it connects

- Integrate `SteadyTimeSource` where that responsibility belongs in the engine boundary.

## Notes

- A monotonic source keeps delta time stable for the game loop.
