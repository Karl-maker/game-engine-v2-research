# FogVolumeComponent

## Purpose

- FogVolumeComponent (descriptive)
- Defines a simple axis-aligned fog/mist volume for the renderer to apply.
- The graphics system collects these and the renderer applies fog in shaders.

## Use when

- Use `FogVolumeComponent` when you need ECS data for fogvolumecomponent behavior.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `sizeMeters` — Size, reach, or distance tuning.
- `color` — Color or tint control.
- `density` — Field of type `float` used by systems that consume this component.
- `startDistance` — Size, reach, or distance tuning.
- `endDistance` — Size, reach, or distance tuning.
- `heightFalloff` — Field of type `float` used by systems that consume this component.
- `baseHeightOffset` — Spatial placement or relative offset.

## Example

```cpp
auto& component = registry.emplace<ecs::FogVolumeComponent>(entity);
component = ecs::FogVolumeComponent{};
```

## Notes

- Keep `FogVolumeComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
