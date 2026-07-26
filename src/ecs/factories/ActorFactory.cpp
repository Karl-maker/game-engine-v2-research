// Author: Karl-Johan Bailey

#include "ecs/factories/ActorFactory.h"

#include "ecs/factories/PhysicalObjectFactory.h"

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/AudioComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId ActorFactory::create(
    EntityRegistry& registry,
    const ActorConfig& config) {
  PhysicalObjectFactory objectFactory;
  PhysicalObjectConfig objectCfg{};
  objectCfg.transform = config.transform;
  objectCfg.viewable = config.viewable;
  EntityId id = objectFactory.create(registry, objectCfg);
  if (id == kInvalidEntityId) return id;

  if (config.viewable.meshKey.empty()) {
    return id;
  }

  auto& skeleton = registry.emplace<ecs::SkeletonComponent>(id);
  skeleton.skeletonId = config.viewable.skeletonId;
  skeleton.skeletonData = config.skeleton.skeletonData;
  skeleton.updateMode = ecs::SkeletonComponent::UpdateMode::WhenVisible;

  auto& motion = registry.emplace<ecs::MotionComponent>(id);
  motion.mode = ecs::MotionComponent::Mode::Walking;
  motion.isGrounded = false;

  auto& stats = registry.emplace<ecs::StatsComponent>(id);
  stats.walkingSpeed = stats.walkingSpeed;
  stats.runningSpeed = stats.runningSpeed;
  stats.baseAttack = stats.baseAttack;
  stats.defense = stats.defense;
  stats.health = stats.health;
  stats.maxHealth = stats.maxHealth;
  stats.specialAttack = stats.specialAttack;
  stats.speed = stats.speed;
  stats.swimmingSpeed = stats.swimmingSpeed;

  auto& audio = registry.emplace<ecs::AudioComponent>(id);
  audio.clipKey = "";      // e.g. "assets/audio/footstep.wav"
  audio.loop = false;
  audio.playOnStart = false;
  audio.volume = 1.0f;
  audio.pitch = 1.0f;

  // @TODO - Add Combat Volume which attaches to skeleton points
  

  return id;


}

}  // namespace ecs::services
