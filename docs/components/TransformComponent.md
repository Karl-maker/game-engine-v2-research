# TransformComponent

## What it is

- TransformComponent
- Any element can have a location/orientation/scale in the world.
- Attach it to an entity to give it spatial data.

## When to use it

- Use this on nearly every world object that needs a place in 3D space.

## How it fits

- Treat `TransformComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set position first, then adjust rotation and scale so downstream systems can interpret the space cleanly.

## Common recipes

- **World prop:** keep scale at `(1, 1, 1)` and only set position and rotation.
- **Spawned character:** place the actor at the desired world position, then let movement and camera systems update it.
- **World anchor:** use this as the parent position for HUD billboards, sockets, or attached effects.

## Field guide

- `position` — Spatial placement or directional tuning.
- `rotation` — Orientation or angular tuning.
- `scale` — Size, reach, or distance tuning.

## Example

```cpp
auto& component = registry.emplace<ecs::TransformComponent>(entity);
component = ecs::TransformComponent{};
```

## Common pairings

- Pair `TransformComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `TransformComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
