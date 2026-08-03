#pragma once

// Author: Karl-Johan Bailey
//
// PlayableCharacterFactory
// Spawns a player-controlled character and configures an existing camera entity
// for third-person follow.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct PlayableCharacterCameraConfig final {
  // Camera entity to configure. If invalid, the factory skips camera setup.
  EntityId cameraEntity = kInvalidEntityId;

  TransformInput transform{.name = "camera"};

  // Third person follow tuning.
  math::Vec3 targetOffset{0.0f, 1.8f, 0.0f};
  float distance = 1.7f;
  float height = 0.50f;
  float pitchDeg = 5.0f;
  float minPitchDeg = -30.0f;
  float maxPitchDeg = 45.0f;

  // Post effects.
  bool depthOfFieldEnabled = true;
  float dofFocusRange = 0.25f;
  float dofBlurStrength = 0.05f;
  math::Vec3 dofFocusTargetOffset{0.0f, 1.6f, 0.0f};

  bool motionBlurEnabled = true;
  float motionBlurStrength = 0.05f;
  float motionBlurMaxBlurPixels = 8.0f;
  int motionBlurSamples = 12;
};

struct PlayableCharacterConfig final {
  CombatantConfig base{};

  // Adds a ControllerComponent (player input interface).
  bool hasController = true;

  // Screen HUD setup for the local player.
  PlayerHudInput hud{};

  PlayableCharacterCameraConfig camera{};
};

class PlayableCharacterFactory final {
 public:
  EntityId create(EntityRegistry& registry, const PlayableCharacterConfig& config);
};

}  // namespace ecs::services
