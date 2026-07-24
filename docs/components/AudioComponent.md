# AudioComponent

## Purpose

- AudioComponent (data + play requests)
- Describes a single audio clip attached to an entity. AudioSystem interprets this.

## Use when

- Use `AudioComponent` when an entity should request sound playback or carry audio clip settings.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `clipKey` — Field of type `std::string` used by systems that consume this component.
- `loop` — Field of type `bool` used by systems that consume this component.
- `playOnStart` — Field of type `bool` used by systems that consume this component.
- `requestPlay` — Field of type `bool` used by systems that consume this component.
- `requestStop` — Field of type `bool` used by systems that consume this component.
- `restartIfPlaying` — Field of type `bool` used by systems that consume this component.
- `volume` — Field of type `float` used by systems that consume this component.
- `pitch` — Field of type `float` used by systems that consume this component.
- `durationSeconds` — Field of type `float` used by systems that consume this component.
- `playing` — Field of type `bool` used by systems that consume this component.
- `playheadSeconds` — Field of type `float` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::AudioComponent>(entity);
component = ecs::AudioComponent{};
```

## Notes

- Set `requestPlay` or `requestStop` from gameplay code, then let the audio system clear those flags after consuming them.
- Use `playOnStart` for ambient or looped clips that should begin as soon as the entity spawns.
