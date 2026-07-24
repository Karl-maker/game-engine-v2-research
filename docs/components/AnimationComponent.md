# AnimationComponent

## What it is

- AnimationComponent stores data that systems and factories read to drive animation and pose.

## When to use it

- Use this when an entity should play animation clips and blend between states.

## How it fits

- Treat `AnimationComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Start with a base layer and give it the state you want visible by default.
- Fill `availableClips` from your model loader or authoring pipeline.
- Use `layers` when you need overrides, additive motion, or masked upper-body animation.

## Enums

- `BlendMode`: `Override`, `Additive`

## Field guide

- `name` — Stable reference used by content, loaders, or rendering systems.
- `weight` — Numeric tuning used by gameplay or rendering systems.
- `blendMode` — Enum or bitmask value that changes system behavior.
- `mask` — Enum or bitmask value that changes system behavior.
- `currentState` — Field of type `std::string` consumed by systems that read this component.
- `nextState` — Field of type `std::string` consumed by systems that read this component.
- `transition` — Field of type `float` consumed by systems that read this component.
- `enabled` — Master on/off switch or similar behavior flag.
- `currentFrame` — Field of type `float` consumed by systems that read this component.
- `availableClips` — Stable reference used by content, loaders, or rendering systems.
- `layers` — Enum or bitmask value that changes system behavior.
- `idleElapsedSeconds` — Stable reference used by content, loaders, or rendering systems.
- `idleDelaySeconds` — Stable reference used by content, loaders, or rendering systems.
- `idleAnimationClip` — Stable reference used by content, loaders, or rendering systems.
- `idleAnimationLayer` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& animation = registry.emplace<ecs::AnimationComponent>(entity);
animation.availableClips = {"Idle", "Walk", "Run"};
animation.layers.push_back({"Base Layer", 1.0f, ecs::AnimationComponent::BlendMode::Override, {}, "Idle", "", 0.0f});
```

## Common pairings

- Pair this with `SkeletonComponent` and `PoseComponent` for skinned characters.
- Keep idle timing values tuned so the system can return to rest poses naturally.

## Notes

- Keep `AnimationComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
