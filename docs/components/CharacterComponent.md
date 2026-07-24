# CharacterComponent

## Purpose

- CharacterComponent marks a humanoid or character-style actor and stores lightweight turn tuning.

## Use when

- Use `CharacterComponent` when an entity should be treated as a character by movement, animation, combat, or input systems.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `turnSpeedDegPerSecond` — Movement or force tuning.

## Example

```cpp
auto& component = registry.emplace<ecs::CharacterComponent>(entity);
component = ecs::CharacterComponent{};
```

## Notes

- Pair this with `ControllerComponent`, `MotionComponent`, `StatsComponent`, and `AnimationComponent` for a full playable actor.
- Keep `CharacterComponent` focused on data so systems can stay deterministic and easy to extend.
