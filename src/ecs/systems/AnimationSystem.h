#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class AnimationSystem final {
 public:
  void tick(EntityRegistry& registry, double deltaSeconds) const;
};

}  // namespace ecs::systems
