# PhysicsMaterialComponent

## What it is

- PhysicsMaterialComponent (descriptive only)
- Describes surface behavior for physics interactions.
- Examples:
- - Ice:    friction=0.02, restitution=0.0
- - Rubber: friction=1.0,  restitution=0.8

## When to use it

- Use this when different surfaces or bodies should react differently on contact.

## How it fits

- Treat `PhysicsMaterialComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Match the material to the gameplay feel you want: sticky, slippery, bouncy, or neutral.

## Field guide

- `friction` — Field of type `float` consumed by systems that read this component.
- `restitution` — Field of type `float` consumed by systems that read this component.
- `rollingFriction` — Orientation or angular tuning.
- `hasDensity` — Numeric tuning used by gameplay or rendering systems.
- `density` — Numeric tuning used by gameplay or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::PhysicsMaterialComponent>(entity);
component = ecs::PhysicsMaterialComponent{};
```

## Common pairings

- Pair `PhysicsMaterialComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `PhysicsMaterialComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
