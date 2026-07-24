#pragma once

// Author: Karl-Johan Bailey
//
// HudSystem
// Updates HUD widget values from other ECS components (for example, player health).

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class HudSystem final {
 public:
  void tick(EntityRegistry& registry);
};

}  // namespace ecs::systems

