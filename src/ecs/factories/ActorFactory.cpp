// Author: Karl-Johan Bailey

#include "ecs/factories/ActorFactory.h"

#include "ecs/factories/PhysicalObjectFactory.h"

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/CombatVolumeComponent.h"
#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/RaycastComponent.h"
#include "ecs/components/SensorComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/SocketComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/AudioComponent.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/IKComponent.h"
#include "ecs/components/PoseComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/VfxComponent.h"
#include "ecs/services/IKService.h"
#include "ecs/services/RaycastConeFactoryService.h"

#include <algorithm>

namespace ecs::services {

namespace {

void applyMount(EntityRegistry& registry, EntityId child, EntityId owner, const AttachmentMountInput& mount) {
  const std::string mode = mount.mode;
  if (mode == "bone" || mode == "socket") {
    auto& socket = registry.emplace<ecs::SocketComponent>(child);
    socket.name = mount.socketName.empty() ? registry.identity(child).name : mount.socketName;
    socket.targetEntity = mount.targetEntity == kInvalidEntityId ? owner : mount.targetEntity;
    socket.targetEntityName = mount.targetEntityName;
    socket.targetMeshId = mount.targetMeshId;
    socket.skeletonName = mount.skeletonId;
    socket.boneName = mount.boneName;
    socket.positionOffset = mount.positionOffset;
    socket.rotationOffset = mount.rotationOffset;
    socket.scaleOffset = mount.scaleOffset;
    return;
  }

  auto& attachment = registry.emplace<ecs::AttachmentComponent>(child);
  ecs::AttachmentComponent::Attachment a{};
  a.targetEntity = mount.targetEntity == kInvalidEntityId ? owner : mount.targetEntity;
  a.targetEntityName = mount.targetEntityName;
  a.targetMeshId = mount.targetMeshId;
  a.mode = ecs::AttachmentComponent::Mode::Parent;
  a.positionOffset = mount.positionOffset;
  a.rotationOffset = mount.rotationOffset;
  a.scaleOffset = mount.scaleOffset;
  a.inheritPosition = mount.inheritPosition;
  a.inheritRotation = mount.inheritRotation;
  a.inheritScale = mount.inheritScale;
  attachment.attachments.push_back(a);
}

void spawnCombatAttachments(EntityRegistry& registry, EntityId owner, const CombatSetupInput& combat) {
  for (const auto& attachmentCfg : combat.volumes) {
    const EntityId child = registry.createEntity(attachmentCfg.name);
    if (child == kInvalidEntityId) continue;
    registry.emplace<ecs::TransformComponent>(child);
    applyMount(registry, child, owner, attachmentCfg.mount);
    auto& combatComp = registry.emplace<ecs::CombatVolumeComponent>(child);
    combatComp.ownerEntity = owner;
    combatComp.attachmentKey = attachmentCfg.name;
    combatComp.sourceMeshId = attachmentCfg.sourceMeshId;
    combatComp.volumes = attachmentCfg.volumes;
  }

  for (const auto& rayCfg : combat.raycasts) {
    const EntityId child = registry.createEntity(rayCfg.name);
    if (child == kInvalidEntityId) continue;
    registry.emplace<ecs::TransformComponent>(child);
    applyMount(registry, child, owner, rayCfg.mount);
    if (rayCfg.createSensor) {
      auto& sensor = registry.emplace<ecs::SensorComponent>(child);
      sensor.parentEntity = owner;
      sensor.enabled = true;
    }
    auto ray = rayCfg.raycast;
    ray.ownerEntity = owner;
    ray.originEntity = owner;
    if (rayCfg.createSensor) ray.sensorEntity = child;
    registry.emplace<ecs::RaycastComponent>(child) = ray;
  }

  for (const auto& vfxCfg : combat.vfx) {
    const EntityId child = registry.createEntity(vfxCfg.name);
    if (child == kInvalidEntityId) continue;
    registry.emplace<ecs::TransformComponent>(child);
    applyMount(registry, child, owner, vfxCfg.mount);
    registry.emplace<ecs::VfxComponent>(child) = vfxCfg.vfx;
  }
}

}  // namespace

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
    pose.enabled = true;
    pose.poses.reserve(config.pose.poses.size() + (config.pose.defaultPoseName.empty() ? 0u : 1u));
    for (const auto& poseCfg : config.pose.poses) {
      ecs::PoseComponent::Pose out{};
      out.name = poseCfg.name;
      out.enabled = poseCfg.enabled;
      out.weight = poseCfg.weight;
      out.bones.reserve(poseCfg.bones.size());
      for (const auto& boneCfg : poseCfg.bones) {
        ecs::PoseComponent::BoneOverride bone{};
        bone.boneKey = boneCfg.boneKey;
        bone.weight = boneCfg.weight;
        bone.hasTranslation = boneCfg.hasTranslation;
        bone.translation = boneCfg.translation;
        bone.hasRotationEulerDeg = boneCfg.hasRotationEulerDeg;
        bone.rotationEulerDeg = boneCfg.rotationEulerDeg;
        bone.hasRotationQuat = boneCfg.hasRotationQuat;
        bone.rotation = boneCfg.rotation;
        bone.hasScale = boneCfg.hasScale;
        bone.scale = boneCfg.scale;
        out.bones.push_back(std::move(bone));
      }
      pose.poses.push_back(std::move(out));
    }
    if (!config.pose.defaultPoseName.empty()) {
      const auto existing = std::find_if(pose.poses.begin(), pose.poses.end(), [&](const ecs::PoseComponent::Pose& p) {
        return p.name == config.pose.defaultPoseName;
      });
      if (existing == pose.poses.end()) {
        pose.poses.push_back(ecs::PoseComponent::Pose{
            .name = config.pose.defaultPoseName,
            .enabled = config.pose.defaultPoseEnabled,
            .weight = config.pose.defaultPoseWeight,
            .bones = {},
        });
      }
    }
  }

  if (config.animation.enabled) {
    auto& anim = registry.emplace<ecs::AnimationComponent>(id);
    anim.enabled = true;
    anim.availableClips = config.animation.availableClips;
    anim.clipBindings.reserve(config.animation.clipBindings.size());
    for (const auto& binding : config.animation.clipBindings) {
      anim.clipBindings.push_back({binding.key, binding.clip, binding.speed});
    }
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
    anim.idleAnimationKey = config.animation.idleAnimationKey;
    anim.idleAnimationLayer = config.animation.idleAnimationLayer;
    anim.locomotionIdleKey = config.animation.locomotionIdleKey;
    anim.locomotionWalkKey = config.animation.locomotionWalkKey;
    anim.locomotionRunKey = config.animation.locomotionRunKey;
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
    cfg.skeletonName = config.sensorCone.skeletonName;
    cfg.socketPositionOffset = config.sensorCone.socketPositionOffset;
    cfg.socketRotationOffset = config.sensorCone.socketRotationOffset;
    cfg.socketScaleOffset = config.sensorCone.socketScaleOffset;
    cfg.cone.baseName = config.sensorCone.cone.baseName;
    cfg.cone.rayCount = config.sensorCone.cone.rayCount;
    cfg.cone.coneAngleDeg = config.sensorCone.cone.coneAngleDeg;
    cfg.cone.length = config.sensorCone.cone.length;
    cfg.cone.radius = config.sensorCone.cone.radius;
    cfg.cone.collisionLayers = config.sensorCone.cone.collisionLayers;
    cfg.cone.ignoreLayers = config.sensorCone.cone.ignoreLayers;
    cfg.cone.ignoreTriggerColliders = config.sensorCone.cone.ignoreTriggerColliders;
    cfg.cone.ignoreSelf = config.sensorCone.cone.ignoreSelf;
    cfg.cone.maxHits = config.sensorCone.cone.maxHits;
    cfg.cone.originLocalOffset = config.sensorCone.cone.originLocalOffset;
    rayFactory.createSensorCone(registry, id, cfg);
  }

  spawnCombatAttachments(registry, id, config.combat);

  return id;
}

}  // namespace ecs::services
