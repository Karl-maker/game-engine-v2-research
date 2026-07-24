# SensorComponent

## Purpose

- SensorComponent (descriptive only)
- Represents a "sensor" entity that is logically a child of a parent entity.
- Example:
- - `enemy_3` (parent entity)
- - `eye_sensor_enemy_3` (sensor entity) with:
- SensorComponent.parentEntity = enemy_3
- AttachmentComponent targeting enemy_3 (so the sensor transform follows)
- - The sensor can own multiple raycasts by creating child entities that each have a RaycastComponent
- (or by using RaycastConeFactoryService to create many raycast entities).

## Use when

- Use `SensorComponent` when you need ECS data for sensorcomponent behavior.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `parentEntity` — Field of type `EntityId` used by systems that consume this component.
- `raycastEntities` — Field of type `std::vector<EntityId>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::SensorComponent>(entity);
component = ecs::SensorComponent{};
```

## Notes

- Keep `SensorComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
