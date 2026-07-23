#pragma once

// Author: Karl-Johan Bailey
//
// RaycastConeFactoryService (descriptive only)
// Creates a cone-shaped set of raycast entities:
// - Each raycast is its own entity with a RaycastComponent
// - Each raycast entity is attached to the owner via AttachmentComponent (inherits rotation)
//
// A raycast system can later use:
// - the raycast entity's attachment-derived transform (origin/orientation)
// - RaycastComponent parameters (length, radius, layers, etc)

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "math/Vec3.h"
#include "physics/LayerMask.h"

#include <string>
#include <vector>

namespace ecs::services {

struct RaycastConeConfig {
  int rayCount = 16;
  float coneAngleDeg = 25.0f;

  float length = 5.0f;
  float radius = 0.0f;  // 0 = ray, >0 = sphere cast

  physics::LayerMask collisionLayers = physics::kAllLayers;
  physics::LayerMask ignoreLayers = 0;

  bool ignoreTriggerColliders = true;
  bool ignoreSelf = true;
  int maxHits = 1;

  // Attachment offsets: where the cone originates relative to the owner.
  math::Vec3 originLocalOffset{0.0f, 0.0f, 0.0f};

  // Naming for created ray entities.
  std::string baseName = "cone_raycast";
};

struct SensorConeConfig {
  std::string sensorName = "head_sensor";
  std::string socketName = "head_socket";

  std::string skeletonName;

  math::Vec3 socketPositionOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 socketRotationOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 socketScaleOffset{0.0f, 0.0f, 0.0f};

  RaycastConeConfig cone{};
};

class RaycastConeFactoryService final {
 public:
  // Creates raycast entities attached to `owner`.
  // Returns the created entity ids (caller owns them and may destroy later).
  std::vector<EntityId> createCone(EntityRegistry& registry, EntityId owner, const RaycastConeConfig& cfg);

  // Creates a socket-backed sensor entity on a character bone and attaches a cone of rays to it.
  EntityId createSensorCone(EntityRegistry& registry, EntityId targetEntity, const SensorConeConfig& cfg);
};

}  // namespace ecs::services
