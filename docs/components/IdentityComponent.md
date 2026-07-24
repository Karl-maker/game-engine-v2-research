# IdentityComponent

## What it is

- IdentityComponent
- - A stable numeric id (monotonic, +1 allocation).
- - A human-friendly name.
- This is intended to be serializable (DB/config) and is safe to include on every entity.

## When to use it

- Use this whenever you want a named entity that tools, logs, and UI can refer to.

## How it fits

- Treat `IdentityComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set a clear name in factories so debug output stays readable.

## Field guide

- `id` — Stable reference used by content, loaders, or rendering systems.
- `name` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::IdentityComponent>(entity);
component = ecs::IdentityComponent{};
```

## Common pairings

- Pair `IdentityComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `IdentityComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
