# StatsComponent

## What it is

- StatsComponent (lightweight)
- Used for actors and destructibles (players, NPCs, enemies, barrels, etc).
- Systems read these values to calculate movement and damage.
- Many values may be 0 to mean "not applicable" for a given entity type.
- Health reaching 0 is handled by a system such as destruction, death animation, or a scripted state change.

## When to use it

- Use this for actors, enemies, and destructibles that need tunable gameplay values.

## How it fits

- Treat `StatsComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set `maxHealth` and `health` together for anything that can take damage.
- Fill in movement values if the entity should be driven by locomotion systems.

## Common recipes

- **Player:** set `maxHealth`, `health`, `walkingSpeed`, and `runningSpeed` at spawn time.
- **Enemy:** give the entity a smaller health pool and tune `baseAttack` or `specialAttack` for the encounter.
- **Destructible prop:** set health only, then let a damage or destruction system remove the entity when health reaches zero.

## Field guide

- `health` — Numeric tuning used by gameplay or rendering systems.
- `maxHealth` — Numeric tuning used by gameplay or rendering systems.
- `defense` — Field of type `float` consumed by systems that read this component.
- `baseAttack` — Field of type `float` consumed by systems that read this component.
- `specialAttack` — Field of type `float` consumed by systems that read this component.
- `speed` — Numeric tuning used by gameplay or rendering systems.
- `walkingSpeed` — Numeric tuning used by gameplay or rendering systems.
- `runningSpeed` — Numeric tuning used by gameplay or rendering systems.
- `swimmingSpeed` — Numeric tuning used by gameplay or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::StatsComponent>(entity);
component = ecs::StatsComponent{};
```

## Common pairings

- Zero can mean “not applicable” when a stat should be ignored for that entity.
- Keep combat and movement tuning here so gameplay remains data-driven instead of hard-coded in multiple systems.

## Notes

- Keep `StatsComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
