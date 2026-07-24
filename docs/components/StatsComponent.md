# StatsComponent

## Purpose

- StatsComponent (lightweight)
- Used for actors and destructibles (players, NPCs, enemies, barrels, etc).
- Systems read these values to calculate movement and damage.
- Notes:
- - Many values may be 0 to mean "not applicable" for a given entity type.
- - Health reaching 0 is handled by a system (e.g., destroy entity, play death animation, etc).

## Use when

- Use this for life, combat tuning, and movement stats that gameplay systems can read quickly.

## Key fields

- `health` — Health-related gameplay data.
- `maxHealth` — Health-related gameplay data.
- `defense` — Field of type `float` used by systems that consume this component.
- `baseAttack` — Field of type `float` used by systems that consume this component.
- `specialAttack` — Field of type `float` used by systems that consume this component.
- `speed` — Movement or force tuning.
- `walkingSpeed` — Movement or force tuning.
- `runningSpeed` — Movement or force tuning.
- `swimmingSpeed` — Movement or force tuning.

## Example

```cpp
auto& stats = registry.emplace<ecs::StatsComponent>(entity);
stats.maxHealth = 100.0f;
stats.health = 100.0f;
stats.walkingSpeed = 4.5f;
```

## Notes

- Zero values are valid when a stat is not needed for a particular entity type.
