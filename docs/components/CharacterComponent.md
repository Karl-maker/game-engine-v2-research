# CharacterComponent

## What it is

- CharacterComponent stores data that systems and factories read to drive character and control.

## When to use it

- Use this when an entity should behave like a player, NPC, enemy, or humanoid actor.

## How it fits

- Treat `CharacterComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Pair it with controller, stats, animation, and motion components for a full playable actor.
- Use the turn speed to control how quickly the actor can rotate toward input or targets.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `turnSpeedDegPerSecond` — Numeric tuning used by gameplay or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::CharacterComponent>(entity);
component = ecs::CharacterComponent{};
```

## Common pairings

- This component is intentionally lightweight; systems and factories do the real work.

## Notes

- Keep `CharacterComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
