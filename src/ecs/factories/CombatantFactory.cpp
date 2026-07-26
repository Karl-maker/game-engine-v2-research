// Author: Karl-Johan Bailey

#include "ecs/factories/CombatantFactory.h"

#include "ecs/components/AudioComponent.h"
#include "ecs/components/CharacterComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId CombatantFactory::create(
    EntityRegistry& registry,
    const CombatantConfig& config) {
  EntityId id = registry.createEntity(config.transform.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.transform.position;
  tr.rotation = config.transform.rotationDeg;
  tr.scale = config.transform.scale;

  if (!config.viewable.meshKey.empty()) {
    auto& mesh = registry.emplace<ecs::MeshComponent>(id);
    mesh.meshId = config.viewable.meshId.empty() ? config.transform.name : config.viewable.meshId;
    mesh.meshData.enabled = true;
    mesh.meshData.key = config.viewable.meshKey;
    mesh.meshType = config.viewable.meshType;
    mesh.scale = config.viewable.meshScale;
    mesh.skeletonId = config.viewable.skeletonId;
    mesh.visible = config.viewable.visible;
    mesh.castShadows = config.viewable.castShadows;
    mesh.receiveShadows = config.viewable.receiveShadows;
    mesh.tags = config.viewable.tags;

    auto& shader = registry.emplace<ecs::ShaderComponent>(id);
    shader.shader.key = config.viewable.shaderKey;
    shader.castShadows = config.viewable.castShadows;
    shader.receiveShadows = config.viewable.receiveShadows;
    shader.textures = config.viewable.textures;
    shader.parameters = config.viewable.parameters;
    shader.lodBreakpoints.reserve(config.viewable.lodBreakpoints.size());
    for (const auto& bp : config.viewable.lodBreakpoints) {
      ecs::ShaderComponent::LodBreakpoint out{};
      out.distanceMeters = bp.distanceMeters;
      out.textures = bp.textures;
      out.parameters = bp.parameters;
      out.overrideTessellation = bp.overrideTessellation;
      out.tessNear = bp.tessNear;
      out.tessFar = bp.tessFar;
      out.tessMin = bp.tessMin;
      out.tessMax = bp.tessMax;
      out.tessQuality = bp.tessQuality;
      shader.lodBreakpoints.push_back(std::move(out));
    }
  }

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

  registry.emplace<ecs::CharacterComponent>(id);

  auto& skeleton = registry.emplace<ecs::SkeletonComponent>(id);
  skeleton.skeletonId = config.viewable.skeletonId;
  skeleton.skeletonData = config.skeleton.skeletonData;
  skeleton.updateMode = ecs::SkeletonComponent::UpdateMode::WhenVisible;

  auto& motion = registry.emplace<ecs::MotionComponent>(id);
  motion.mode = ecs::MotionComponent::Mode::Walking;
  motion.isGrounded = false;

  auto& stats = registry.emplace<ecs::StatsComponent>(id);
  stats.walkingSpeed = config.stats.walkingSpeed;
  stats.runningSpeed = config.stats.runningSpeed;
  stats.baseAttack = config.stats.baseAttack;
  stats.defense = config.stats.defense;
  stats.health = config.stats.health;
  stats.maxHealth = config.stats.maxHealth;
  stats.specialAttack = config.stats.specialAttack;
  stats.speed = config.stats.speed;
  stats.swimmingSpeed = config.stats.swimmingSpeed;

  auto& audio = registry.emplace<ecs::AudioComponent>(id);
  audio.clipKey = "";
  audio.loop = false;
  audio.playOnStart = false;
  audio.volume = 1.0f;
  audio.pitch = 1.0f;

  return id;
}

}  // namespace ecs::services
