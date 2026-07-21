#pragma once

// Author: Karl-Johan Bailey
//
// TargetComponent (orientation only)
// Describes how an entity should orient toward a target.
//
// Notes:
// - This component is descriptive only; a system applies the rotation.
// - It does not move the entity, it only affects orientation.

#include "ecs/EntityId.h"
#include "math/Vec3.h"

namespace ecs {

struct TargetComponent {
  EntityId targetEntity = kInvalidEntityId;
  math::Vec3 targetOffset{0.0f, 0.0f, 0.0f};

  // Typically (0,1,0). Included for completeness; systems may use it for full look-at math.
  math::Vec3 upVector{0.0f, 1.0f, 0.0f};

  // Degrees per second to rotate toward the target.
  float rotationSpeed = 180.0f;

  // Keep roll at 0 when true (common for cameras).
  bool lockRoll = true;

  bool enabled = true;
};

}  // namespace ecs

