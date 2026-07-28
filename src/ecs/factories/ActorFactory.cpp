// Author: Karl-Johan Bailey

#include "ecs/factories/ActorFactory.h"

#include "ecs/factories/PhysicalObjectFactory.h"

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/AudioComponent.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/IKComponent.h"
#include "ecs/components/PoseComponent.h"
#include "ecs/services/IKService.h"
#include "ecs/services/RaycastConeFactoryService.h"

#include <algorithm>

namespace ecs::services {

EntityId ActorFactory::create(
    EntityRegistry& registry,
    const ActorConfig& config) {
  PhysicalObjectFactory objectFactory;
  PhysicalObjectConfig objectCfg{};
  objectCfg.transform = config.transform;
  objectCfg.viewable = config.viewable;
  objectCfg.physical = config.physical;
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
  audio.clipKey = "";      // e.g. "assets/audio/footstep.wav"
  audio.loop = false;
  audio.playOnStart = false;
  audio.volume = 1.0f;
  audio.pitch = 1.0f;

  if (config.pose.enabled) {
    auto& pose = registry.emplace<ecs::PoseComponent>(id);
    if (!config.pose.defaultPoseName.empty()) {
      pose.poses.push_back(ecs::PoseComponent::Pose{
          .name = config.pose.defaultPoseName,
          .enabled = config.pose.defaultPoseEnabled,
          .weight = config.pose.defaultPoseWeight,
          .bones = {},
      });
    }
  }

  if (config.animation.enabled) {
    auto& anim = registry.emplace<ecs::AnimationComponent>(id);
    anim.enabled = true;
    anim.availableClips = config.animation.availableClips;
    anim.layers.clear();
    anim.layers.reserve(config.animation.layers.size());
    for (const auto& l : config.animation.layers) {
      ecs::AnimationComponent::AnimationLayer out{};
      out.name = l.name;
      out.weight = l.weight;
      out.blendMode = (l.blendMode == "Additive" || l.blendMode == "additive")
                          ? ecs::AnimationComponent::BlendMode::Additive
                          : ecs::AnimationComponent::BlendMode::Override;
      out.mask = l.mask;
      out.currentState = l.currentState;
      out.nextState = l.nextState;
      out.transition = l.transition;
      anim.layers.push_back(std::move(out));
    }
    anim.idleDelaySeconds = config.animation.idleDelaySeconds;
    anim.idleAnimationClip = config.animation.idleAnimationClip;
    anim.idleAnimationLayer = config.animation.idleAnimationLayer;
  }

  if (config.ik.enabled) {
    auto& ik = registry.emplace<ecs::IKComponent>(id);
    ik.enabled = true;

    if (config.ik.headLook.enabled && !config.ik.headLook.name.empty()) {
      auto& chain = IKService::ensureChain(ik, config.ik.headLook.name, config.ik.headLook.bones);
      chain.overrideAnimation = config.ik.headLook.overrideAnimation;
      IKService::setEntityTarget(chain, config.ik.headLook.targetEntity, config.ik.headLook.targetOffset);
      chain.targetLocalOffset = config.ik.headLook.targetLocalOffset;
      IKService::setWeight(chain, config.ik.headLook.weight);
      IKService::setBlendTimes(chain, config.ik.headLook.blendInSeconds, config.ik.headLook.blendOutSeconds);
      IKService::setIterations(chain, config.ik.headLook.iterations);
    }

    if (config.ik.reachTarget.enabled && !config.ik.reachTarget.name.empty()) {
      auto& chain = IKService::ensureChain(ik, config.ik.reachTarget.name, config.ik.reachTarget.bones);
      chain.overrideAnimation = config.ik.reachTarget.overrideAnimation;
      chain.enabled = false;
      IKService::setEntityTarget(chain, config.ik.reachTarget.targetEntity, config.ik.reachTarget.targetOffset);
      chain.targetLocalOffset = config.ik.reachTarget.targetLocalOffset;
      IKService::setWeight(chain, config.ik.reachTarget.weight);
      IKService::setBlendTimes(chain, config.ik.reachTarget.blendInSeconds, config.ik.reachTarget.blendOutSeconds);
      IKService::setIterations(chain, config.ik.reachTarget.iterations);
    }
  }

  if (config.sensorCone.enabled) {
    RaycastConeFactoryService rayFactory;
    SensorConeConfig cfg;
    cfg.sensorName = config.sensorCone.sensorName;
    cfg.socketName = config.sensorCone.socketName;
    cfg.socketPositionOffset = config.sensorCone.socketPositionOffset;
    cfg.cone.baseName = config.sensorCone.cone.baseName;
    cfg.cone.rayCount = config.sensorCone.cone.rayCount;
    cfg.cone.coneAngleDeg = config.sensorCone.cone.coneAngleDeg;
    cfg.cone.length = config.sensorCone.cone.length;
    cfg.cone.radius = config.sensorCone.cone.radius;
    cfg.cone.collisionLayers = config.sensorCone.cone.collisionLayers;
    cfg.cone.ignoreLayers = config.sensorCone.cone.ignoreLayers;
    cfg.cone.ignoreSelf = config.sensorCone.cone.ignoreSelf;
    cfg.cone.maxHits = config.sensorCone.cone.maxHits;
    cfg.cone.originLocalOffset = config.sensorCone.cone.originLocalOffset;
    rayFactory.createSensorCone(registry, id, cfg);
  }

  // @TODO - Add Combat Volume which attaches to skeleton points

  return id;


}

}  // namespace ecs::services
