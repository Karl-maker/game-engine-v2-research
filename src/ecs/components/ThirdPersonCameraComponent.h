#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "math/Vec3.h"

namespace ecs {

struct ThirdPersonCameraComponent final {
  bool enabled = true;
  EntityId target = kInvalidEntityId;
  math::Vec3 targetOffset{0.0f, 1.35f, 0.0f};
  float distance = 4.8f;
  float height = 1.1f;
  float pitchDeg = 12.0f;
  float minPitchDeg = -12.0f;
  float maxPitchDeg = 42.0f;
  float yawDeg = 0.0f;
  float followSharpness = 18.0f;
};

}  // namespace ecs
