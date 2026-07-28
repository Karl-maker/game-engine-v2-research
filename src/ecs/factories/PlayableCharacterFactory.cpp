// Author: Karl-Johan Bailey

#include "ecs/factories/PlayableCharacterFactory.h"

#include "ecs/components/CameraComponent.h"
#include "ecs/components/ControllerComponent.h"
#include "ecs/components/ThirdPersonCameraComponent.h"
#include "ecs/components/TransformComponent.h"

namespace ecs::services {

EntityId PlayableCharacterFactory::create(EntityRegistry& registry, const PlayableCharacterConfig& config) {
  EntityId cameraId = config.camera.cameraEntity;
  if (cameraId == kInvalidEntityId && !config.camera.transform.name.empty()) cameraId = registry.createEntity(config.camera.transform.name);

  CombatantConfig baseCfg = config.base;
  if (baseCfg.ik.enabled && baseCfg.ik.headLook.enabled) {
    baseCfg.ik.headLook.targetEntity = cameraId;
  }

  CombatantFactory baseFactory;
  const EntityId player = baseFactory.create(registry, baseCfg);
  if (player == kInvalidEntityId) return player;

  if (config.hasController) registry.emplace<ecs::ControllerComponent>(player);

  if (cameraId != kInvalidEntityId && registry.isAlive(cameraId)) {
    const EntityId camId = cameraId;

    auto& camTr = registry.emplace<ecs::TransformComponent>(camId);
    camTr.position = config.camera.transform.position;
    camTr.rotation = config.camera.transform.rotationDeg;
    camTr.scale = config.camera.transform.scale;

    auto& cam = registry.emplace<ecs::CameraComponent>(camId);
    cam.depthOfField.enabled = config.camera.depthOfFieldEnabled;
    cam.depthOfField.focusMode = ecs::CameraComponent::DepthOfFieldSettings::FocusMode::TargetEntity;
    cam.depthOfField.focusTarget = player;
    cam.depthOfField.focusTargetOffset = config.camera.dofFocusTargetOffset;
    cam.depthOfField.focusRange = config.camera.dofFocusRange;
    cam.depthOfField.blurStrength = config.camera.dofBlurStrength;

    cam.motionBlur.enabled = config.camera.motionBlurEnabled;
    cam.motionBlur.strength = config.camera.motionBlurStrength;
    cam.motionBlur.maxBlurPixels = config.camera.motionBlurMaxBlurPixels;
    cam.motionBlur.samples = config.camera.motionBlurSamples;

    auto& thirdPerson = registry.emplace<ecs::ThirdPersonCameraComponent>(camId);
    thirdPerson.target = player;
    thirdPerson.targetOffset = config.camera.targetOffset;
    thirdPerson.distance = config.camera.distance;
    thirdPerson.height = config.camera.height;
    thirdPerson.pitchDeg = config.camera.pitchDeg;
    thirdPerson.minPitchDeg = config.camera.minPitchDeg;
    thirdPerson.maxPitchDeg = config.camera.maxPitchDeg;

    if (const auto* playerTr = registry.tryGet<ecs::TransformComponent>(player)) {
      thirdPerson.yawDeg = playerTr->rotation.y;
    }
  }

  return player;
}

}  // namespace ecs::services
