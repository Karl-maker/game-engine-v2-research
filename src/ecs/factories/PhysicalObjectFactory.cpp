// Author: Karl-Johan Bailey

// ActorFactory
// Spawns a lightweight world "actor" (no rigidbody/collider by default).

#include "ecs/factories/PhysicalObjectFactory.h"
#include "ecs/factories/ObjectFactory.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/MotionComponent.h"

#include <algorithm>

namespace ecs::services {

namespace {

std::uint32_t hashString32(const std::string& text) {
  std::uint32_t hash = 2166136261u;
  for (unsigned char ch : text) {
    hash ^= static_cast<std::uint32_t>(ch);
    hash *= 16777619u;
  }
  return hash;
}

}

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
    rb.buoyant = config.physical.buoyant;
    rb.buoyancyHeight = config.physical.buoyancyHeight;
  }

  if (config.physical.hasCollider) {
    auto& collider = registry.emplace<ecs::ColliderComponent>(id);
    collider.shape = config.physical.colliderShape;
    collider.size = config.physical.colliderSize;
    collider.offset = config.physical.colliderOffset;
    collider.isTrigger = config.physical.colliderIsTrigger;
    collider.collisionLayer = config.physical.collisionLayer;
    collider.meshRef = config.physical.colliderMeshId.empty() ? config.viewable.meshId : config.physical.colliderMeshId;
    collider.meshAssetKey = config.physical.colliderMeshKey.empty() ? config.viewable.meshKey : config.physical.colliderMeshKey;
    collider.useMeshBounds = config.physical.colliderUseMeshBounds;
    collider.hasMesh = !collider.meshRef.empty() || !collider.meshAssetKey.empty();
    const std::string meshHashSource = !collider.meshRef.empty() ? collider.meshRef : collider.meshAssetKey;
    collider.meshId = collider.hasMesh ? hashString32(meshHashSource) : 0u;
  }

  // Ensure collidable dynamic objects participate in collision resolution.
  if (config.physical.hasCollider && config.physical.hasRigidbody && !config.physical.kinematic) {
    if (!registry.tryGet<ecs::MotionComponent>(id)) {
      auto& motion = registry.emplace<ecs::MotionComponent>(id);
      motion.mode = ecs::MotionComponent::Mode::Walking;
      motion.isGrounded = false;
    }
  }

  // @TODO - Add Combat Volume for basic objects

  return id;
}

}  // namespace ecs::services
