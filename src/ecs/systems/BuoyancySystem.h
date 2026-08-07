#pragma once

// Author: Karl-Johan Bailey
//
// Buoyancy System
// Rocks things like boats back and forth in the water, not all entities should be rocked
// Checks for items in the water from the WaterDetectionSystem

#include "ecs/EntityRegistry.h"

#include <cstdint>
#include <unordered_set>

namespace ecs::systems {

class BuoyancySystem final {
 public:
  void tick(EntityRegistry& registry, double timeSeconds, double elapsedSeconds) const;
};

}  // namespace ecs::systems
