#pragma once

// Author: Karl-Johan Bailey
//
// CombatantHudSystem
// Ensures combatant HUD entities stay configured to follow their target and render the correct SVG billboard.

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class CombatantHudSystem final {
 public:
  void tick(EntityRegistry& registry) const;
};

}  // namespace ecs::systems

