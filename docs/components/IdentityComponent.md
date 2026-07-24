# IdentityComponent

## Purpose

- IdentityComponent
- - A stable numeric id (monotonic, +1 allocation).
- - A human-friendly name.
- This is intended to be serializable (DB/config) and is safe to include on every entity.

## Use when

- Use `IdentityComponent` when you need ECS data for identitycomponent behavior.

## Key fields

- `id` — A stable content or debug identifier.
- `name` — A stable content or debug identifier.

## Example

```cpp
auto& component = registry.emplace<ecs::IdentityComponent>(entity);
component = ecs::IdentityComponent{};
```

## Notes

- Keep `IdentityComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
