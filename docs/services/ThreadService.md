# ThreadService

## Purpose

- ThreadService provides reusable engine behavior.

## Use when

- Minimal worker pool for background compute and simple parallel-for style work.

## Example

```cpp
core::ThreadService threads;
threads.parallelFor(1000, 64, [](std::size_t i) { (void)i; });
```

## Notes

- Avoid mutating shared ECS state from worker jobs; compute off-thread and commit on the main thread.
