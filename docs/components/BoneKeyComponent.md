# BoneKeyComponent

## Purpose

- BoneKeyComponent stores gameplay or rendering data for ECS systems.

## Use when

- Use `BoneKeyComponent` when you need ECS data for bonekeycomponent behavior.

## Key fields

- `leftHandKey` — Field of type `std::string` used by systems that consume this component.
- `rightHandKey` — Field of type `std::string` used by systems that consume this component.
- `headKey` — Field of type `std::string` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::BoneKeyComponent>(entity);
component = ecs::BoneKeyComponent{};
```

## Notes

- Keep `BoneKeyComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
