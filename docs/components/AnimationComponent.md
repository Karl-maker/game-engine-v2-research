# AnimationComponent

## Purpose

- AnimationComponent stores gameplay or rendering data for ECS systems.

## Use when

- Use this for animated characters, creatures, props, and any entity that plays clip-based motion.

## Enums

- `BlendMode`: `Override`, `Additive`

## Key fields

- `name` — A stable content or debug identifier.
- `weight` — Field of type `float` used by systems that consume this component.
- `blendMode` — Field of type `BlendMode` used by systems that consume this component.
- `mask` — Layer or filtering control.
- `currentState` — Field of type `std::string` used by systems that consume this component.
- `nextState` — Field of type `std::string` used by systems that consume this component.
- `transition` — Field of type `float` used by systems that consume this component.
- `enabled` — Enable or disable this part of the component.
- `currentFrame` — Field of type `float` used by systems that consume this component.
- `availableClips` — Field of type `std::vector<std::string>` used by systems that consume this component.
- `layers` — Layer or filtering control.
- `idleElapsedSeconds` — Field of type `float` used by systems that consume this component.
- `idleDelaySeconds` — Field of type `float` used by systems that consume this component.
- `idleAnimationClip` — Field of type `std::string` used by systems that consume this component.
- `idleAnimationLayer` — Layer or filtering control.

## Example

```cpp
auto& animation = registry.emplace<ecs::AnimationComponent>(entity);
animation.enabled = true;
animation.availableClips = {"Idle", "Walk", "Run"};
animation.layers.push_back({"Base Layer", 1.0f, ecs::AnimationComponent::BlendMode::Override, {}, "Idle", "", 0.0f});
```

## Notes

- Treat `availableClips` as the catalog of animation names the asset loader found.
- Use `layers` when you want a layered animation graph, blend masks, or state transitions.
- Keep idle timing values tuned so the system can auto-pick a rest pose when the actor is inactive.
