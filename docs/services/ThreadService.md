# ThreadService

## What it is

- ThreadService (minimal thread pool)
- Provides a safe place to run heavy compute in parallel. Callers should avoid
- mutating shared ECS state from worker threads; instead compute into local
- outputs and apply on the main thread.

## When to use it

- Use this when expensive work can be computed off-thread and applied later on the main thread.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
core::ThreadService threads;
threads.parallelFor(1000, 64, [](std::size_t i) { (void)i; });
```

## How it connects

- Integrate `ThreadService` where that responsibility belongs in the engine boundary.

## Notes

- Avoid mutating shared ECS state from worker jobs; compute in workers and commit on the main thread.
