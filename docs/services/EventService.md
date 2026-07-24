# EventService

## Purpose

- EventService provides reusable engine behavior.

## Use when

- Stores per-frame events so systems can emit and consume gameplay signals without tight coupling.

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

## Notes

- Clear event buckets once per frame at a stable sync point.
