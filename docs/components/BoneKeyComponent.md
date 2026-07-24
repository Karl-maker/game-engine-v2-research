# BoneKeyComponent

## What it is

- BoneKeyComponent stores data that systems and factories read to drive skeleton and bones.

## When to use it

- Use this when other systems need a stable way to refer to named bones.

## How it fits

- Treat `BoneKeyComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Populate the keys from the same rig or skeleton naming scheme that your model uses.

## Field guide

- `leftHandKey` — Stable reference used by content, loaders, or rendering systems.
- `rightHandKey` — Stable reference used by content, loaders, or rendering systems.
- `headKey` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::BoneKeyComponent>(entity);
component = ecs::BoneKeyComponent{};
```

## Common pairings

- Pair `BoneKeyComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `BoneKeyComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
