// Author: Karl-Johan Bailey

// ActorFactory
// Spawns a lightweight world "actor" (no rigidbody/collider by default).

#include "ecs/factories/PhysicalObjectFactory.h"
#include "ecs/factories/ObjectFactory.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/RigidbodyComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId PhysicalObjectFactory::create(
    EntityRegistry& registry,
    const PhysicalObjectConfig& config) {
  ObjectFactory objectFactory;
  ObjectConfig objectCfg{};
  objectCfg.transform = config.transform;
  objectCfg.viewable = config.viewable;
  EntityId id = objectFactory.create(registry, objectCfg);
  if (id == kInvalidEntityId) return id;

    if (config.physical.hasRigidbody && config.physical.mass > 0.0001f) {
    auto& rb = registry.emplace<ecs::RigidbodyComponent>(id);
    rb.mass = config.physical.mass;
    rb.inverseMass = 1.0f / std::max(0.0001f, rb.mass);
    rb.useGravity = config.physical.useGravity;
    rb.kinematic = config.physical.kinematic;
  }

  if (config.physical.hasCollider) {
    auto& collider = registry.emplace<ecs::ColliderComponent>(id);
    collider.shape = config.physical.colliderShape;
    collider.size = config.physical.colliderSize;
    collider.offset = config.physical.colliderOffset;
    collider.isTrigger = config.physical.colliderIsTrigger;
    collider.collisionLayer = config.physical.collisionLayer;
  }

  // @TODO - Add Combat Volume for basic objects

  return id;
}

}  // namespace ecs::services
