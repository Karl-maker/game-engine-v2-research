// Author: Karl-Johan Bailey

#include "ecs/factories/EntityFactory.h"

#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId EntityFactory::create(EntityRegistry& registry, const EntityConfig& config) {
  EntityId id = registry.createEntity(config.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.position;
  tr.rotation = config.rotationDeg;
  tr.scale = config.scale;

  if (config.hasRigidbody && config.mass > 0.0001f) {
    auto& rb = registry.emplace<ecs::RigidbodyComponent>(id);
    rb.mass = config.mass;
    rb.inverseMass = 1.0f / std::max(0.0001f, rb.mass);
    rb.useGravity = config.useGravity;
  }

  return id;
}

}  // namespace ecs::services

