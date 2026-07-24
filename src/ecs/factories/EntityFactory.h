#pragma once

// Author: Karl-Johan Bailey
//
// EntityFactory
// Simple "spawn an entity" factory used by chunk loading.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "math/Vec3.h"

namespace ecs::services {

struct EntityConfig final {
  std::string name;
  math::Vec3 position{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationDeg{0.0f, 0.0f, 0.0f};
  math::Vec3 scale{1.0f, 1.0f, 1.0f};

  bool hasRigidbody = false;
  float mass = 0.0f;
  bool useGravity = true;
};

class EntityFactory final {
 public:
  EntityId create(EntityRegistry& registry, const EntityConfig& config);
};

}  // namespace ecs::services

