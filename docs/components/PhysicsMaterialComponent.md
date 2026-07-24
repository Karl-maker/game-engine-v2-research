# PhysicsMaterialComponent

## Purpose

- PhysicsMaterialComponent (descriptive only)
- Describes surface behavior for physics interactions.
- Examples:
- - Ice:    friction=0.02, restitution=0.0
- - Rubber: friction=1.0,  restitution=0.8

## Use when

- Use `PhysicsMaterialComponent` when you need ECS data for physicsmaterialcomponent behavior.

## Key fields

- `friction` — Field of type `float` used by systems that consume this component.
- `restitution` — Field of type `float` used by systems that consume this component.
- `rollingFriction` — Field of type `float` used by systems that consume this component.
- `hasDensity` — Field of type `bool` used by systems that consume this component.
- `density` — Field of type `float` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::PhysicsMaterialComponent>(entity);
component = ecs::PhysicsMaterialComponent{};
```

## Notes

- Keep `PhysicsMaterialComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
