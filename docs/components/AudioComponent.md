# AudioComponent

## What it is

- AudioComponent (data + play requests)
- Describes a single audio clip attached to an entity. AudioSystem interprets this.

## When to use it

- Use this when an entity should request a sound effect, loop, or ambient clip.

## How it fits

- Treat `AudioComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set `clipKey` to the file or asset key for the sound you want to play.
- Toggle `requestPlay` and `requestStop` from gameplay code, then let the audio system consume them.
- Use `playOnStart` for ambient loops that should begin as soon as the entity spawns.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `clipKey` — Stable reference used by content, loaders, or rendering systems.
- `loop` — Master on/off switch or similar behavior flag.
- `playOnStart` — Master on/off switch or similar behavior flag.
- `requestPlay` — Master on/off switch or similar behavior flag.
- `requestStop` — Master on/off switch or similar behavior flag.
- `restartIfPlaying` — Master on/off switch or similar behavior flag.
- `volume` — Field of type `float` consumed by systems that read this component.
- `pitch` — Orientation or angular tuning.
- `durationSeconds` — Field of type `float` consumed by systems that read this component.
- `playing` — Field of type `bool` consumed by systems that read this component.
- `playheadSeconds` — Field of type `float` consumed by systems that read this component.

## Example

```cpp
auto& audio = registry.emplace<ecs::AudioComponent>(entity);
audio.clipKey = "assets/audio/footstep.wav";
audio.playOnStart = false;
audio.loop = false;
audio.requestPlay = true;
```

## Common pairings

- Keep playback state in the component so systems can stay deterministic.
- Use a separate transform or spatial component if you later add 3D attenuation.

## Notes

- Keep `AudioComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
