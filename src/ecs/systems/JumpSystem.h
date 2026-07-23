#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class JumpSystem final {
 public:
  float jumpSpeed = 6.5f;

  void tick(EntityRegistry& registry) const;
};

}  // namespace ecs::systems
