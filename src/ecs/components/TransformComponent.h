#pragma once

// Author: Karl-Johan Bailey
//
// TransformComponent
// Any element can have a location/orientation/scale in the world.
// Attach it to an entity to give it spatial data.

#include "math/Vec3.h"

namespace ecs {

struct TransformComponent {
  using Position = math::Vec3;
  using Rotation = math::Vec3;  // pitch/yaw/roll in degrees
  using Scale = math::Vec3;

  Position position{0.0f, 0.0f, 0.0f};
  Rotation rotation{0.0f, 0.0f, 0.0f};
  Scale scale{1.0f, 1.0f, 1.0f};

  static constexpr int kPitch = 0;
  static constexpr int kYaw = 1;
  static constexpr int kRoll = 2;
};

}  // namespace ecs
