# RockScatterComponent

## Purpose

- RockScatterComponent (descriptive only)
- Describes a procedural scatter of small rocks/pebbles for a renderer/system to generate.

## Use when

- Use `RockScatterComponent` when you need ECS data for rockscattercomponent behavior.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `area` — Field of type `math::Vec3` used by systems that consume this component.
- `density` — Field of type `float` used by systems that consume this component.
- `seed` — Field of type `std::uint32_t` used by systems that consume this component.
- `distribution` — Field of type `std::string` used by systems that consume this component.
- `minScale` — Size, reach, or distance tuning.
- `maxScale` — Size, reach, or distance tuning.
- `clumpiness` — Field of type `float` used by systems that consume this component.
- `patchScale` — Size, reach, or distance tuning.
- `castShadows` — Field of type `bool` used by systems that consume this component.
- `receiveShadows` — Field of type `bool` used by systems that consume this component.
- `lodBias` — Level-of-detail or quality control.

## Example

```cpp
auto& component = registry.emplace<ecs::RockScatterComponent>(entity);
component = ecs::RockScatterComponent{};
```

## Notes

- Keep `RockScatterComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
