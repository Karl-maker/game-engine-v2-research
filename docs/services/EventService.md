# EventService

## What it is

- EventService (minimal)
- Stores per-frame events emitted by systems and allows fast "has events of type T?"
- checks so callers can avoid iterating when nothing happened.
- Lifetime:
- - Events are intended to live for one simulation cycle.
- - Call `clear()` once per frame at a safe point (typically start of a tick).

## When to use it

- Use this when you want one system to broadcast an event and another system to consume it later in the same frame.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
ecs::services::EventService events;
events.emit(MyEvent{.value = 1});
if (events.has<MyEvent>()) {
  for (auto e : events.consumeAll<MyEvent>()) {
    (void)e;
  }
}
```

## How it connects

- Integrate `EventService` where that responsibility belongs in the engine boundary.

## Notes

- Call `clear()` once per tick at a safe sync point if you want all events to be frame-local.
