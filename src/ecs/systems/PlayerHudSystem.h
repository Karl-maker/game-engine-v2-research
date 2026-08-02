#pragma once

// Author: Karl-Johan Bailey
//
// PlayerHudSystem
// Ensures a player HUD entity has Draw2DComponent quads configured for screen-space SVG HUD rendering.

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class PlayerHudSystem final {
 public:
  void tick(EntityRegistry& registry) const;
};

}  // namespace ecs::systems

