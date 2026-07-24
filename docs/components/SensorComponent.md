# SensorComponent

## What it is

- SensorComponent (descriptive only)
- Represents a "sensor" entity that is logically a child of a parent entity.
- Example:
- - `enemy_3` (parent entity)
- - `eye_sensor_enemy_3` (sensor entity) with:
- SensorComponent.parentEntity = enemy_3
- AttachmentComponent targeting enemy_3 (so the sensor transform follows)
- - The sensor can own multiple raycasts by creating child entities that each have a RaycastComponent
- (or by using RaycastConeFactoryService to create many raycast entities).

## When to use it

- Use this when an entity should notice nearby actors, targets, or obstacles.

## How it fits

- Treat `SensorComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Keep the sensing rules narrow enough to avoid expensive broad checks every frame.

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `parentEntity` — Reference to another entity or a linked runtime object.
- `raycastEntities` — Field of type `std::vector<EntityId>` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SensorComponent>(entity);
component = ecs::SensorComponent{};
```

## Common pairings

- Pair `SensorComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `SensorComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
