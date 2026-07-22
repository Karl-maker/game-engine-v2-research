#pragma once

// Author: Karl-Johan Bailey
//
// SensorComponent (descriptive only)
// Represents a "sensor" entity that is logically a child of a parent entity.
//
// Example:
// - `enemy_3` (parent entity)
// - `eye_sensor_enemy_3` (sensor entity) with:
//     SensorComponent.parentEntity = enemy_3
//     AttachmentComponent targeting enemy_3 (so the sensor transform follows)
// - The sensor can own multiple raycasts by creating child entities that each have a RaycastComponent
//   (or by using RaycastConeFactoryService to create many raycast entities).

#include "ecs/EntityId.h"

#include <vector>

namespace ecs {

struct SensorComponent {
  bool enabled = true;

  // The logical parent this sensor belongs to (e.g., enemy entity).
  EntityId parentEntity = kInvalidEntityId;

  // Optional: track raycast entities belonging to this sensor (each raycast is typically its own entity).
  // This makes it easy for systems to enumerate all raycasts for a sensor.
  std::vector<EntityId> raycastEntities;
};

}  // namespace ecs

