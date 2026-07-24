# RockScatterComponent

## What it is

- RockScatterComponent (descriptive only)
- Describes a procedural scatter of small rocks/pebbles for a renderer/system to generate.

## When to use it

- Use this for dense rock clusters that should be instantiated from rules instead of authored by hand.

## How it fits

- Treat `RockScatterComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Tie the scatter density and radius to the terrain scale so the result looks natural.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `area` — Field of type `math::Vec3` consumed by systems that read this component.
- `density` — Numeric tuning used by gameplay or rendering systems.
- `seed` — Field of type `std::uint32_t` consumed by systems that read this component.
- `distribution` — Field of type `std::string` consumed by systems that read this component.
- `minScale` — Size, reach, or distance tuning.
- `maxScale` — Size, reach, or distance tuning.
- `clumpiness` — Field of type `float` consumed by systems that read this component.
- `patchScale` — Size, reach, or distance tuning.
- `castShadows` — Master on/off switch or similar behavior flag.
- `receiveShadows` — Master on/off switch or similar behavior flag.
- `lodBias` — Field of type `float` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::RockScatterComponent>(entity);
component = ecs::RockScatterComponent{};
```

## Common pairings

- Pair `RockScatterComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `RockScatterComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
