#pragma once

// Author: Karl-Johan Bailey
//
// RaycastComponent (descriptive only)
// Describes ray/sphere-cast queries and stores hit results for systems to fill in.
//
// This component does not do any physics queries by itself.

#include "debug/Color.h"
#include "ecs/EntityId.h"
#include "math/Vec3.h"
#include "physics/LayerMask.h"
#include "physics/RaycastHit.h"

#include <vector>
#include <string>

namespace ecs {

struct RaycastComponent {
  enum class OriginMode {
    Entity,       // origin comes from `originEntity` (plus local offset)
    WorldPosition // origin uses `worldPosition` directly
  };

  enum class DirectionMode {
    Forward,
    Up,
    Down,
    Right,
    Left,
    CustomVector,
    TowardTarget,
  };

  bool enabled = true;

  // Logical ownership and filtering metadata.
  EntityId sensorEntity = kInvalidEntityId;
  std::string raycastCategory = "default";
  physics::LayerMask raycastLayer = physics::kAllLayers;

  // Origin
  OriginMode originMode = OriginMode::Entity;
  EntityId originEntity = kInvalidEntityId;
  math::Vec3 localOffset{0.0f, 0.0f, 0.0f};
  bool hasWorldPosition = false;
  math::Vec3 worldPosition{0.0f, 0.0f, 0.0f};

  // Direction
  DirectionMode directionMode = DirectionMode::Forward;
  math::Vec3 customDirection{0.0f, 0.0f, 1.0f};  // used when directionMode == CustomVector
  EntityId towardTargetEntity = kInvalidEntityId; // used when directionMode == TowardTarget
  math::Vec3 towardTargetOffset{0.0f, 0.0f, 0.0f};

  // Cast parameters
  float length = 1.0f;
  float radius = 0.0f;  // 0 = raycast, >0 = sphere cast

  // Collision filtering
  physics::LayerMask collisionLayers = physics::kAllLayers;
  physics::LayerMask ignoreLayers = 0;
  bool ignoreTriggerColliders = true;
  bool ignoreSelf = true;

  int maxHits = 1;
  bool continuous = false;

  // Debug draw
  bool debugDraw = false;
  debug::Color debugColor{0.0f, 1.0f, 0.0f, 1.0f};
  float debugDurationSeconds = 0.0f;

  // Results (filled by raycast system)
  std::vector<physics::RaycastHit> hitResults;
};

}  // namespace ecs
