# SteadyTimeSource

## Purpose

- SteadyTimeSource provides reusable engine behavior.

## Use when

- Monotonic wall-clock time source used by the loop and frame pacing code.

## Example

```cpp
core::SteadyTimeSource timeSource;
auto seconds = timeSource.nowSeconds();
```

## Notes

- Use a monotonic source for frame timing so wall clock changes do not distort gameplay deltas.
