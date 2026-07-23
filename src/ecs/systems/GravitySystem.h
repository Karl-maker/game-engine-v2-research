#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "math/Vec3.h"

namespace ecs::systems {

class GravitySystem final {
 public:
  math::Vec3 direction{0.0f, -1.0f, 0.0f};
  float strength = 9.81f;

  void tick(EntityRegistry& registry, double deltaSeconds) const;
};

}  // namespace ecs::systems
