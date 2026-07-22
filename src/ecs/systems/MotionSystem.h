#pragma once

// Author: Karl-Johan Bailey
//
// MotionSystem
// Converts descriptive controller requests into concrete motion intent
// (currently: desired direction and desired velocity).

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class MotionSystem final {
 public:
  void tick(ecs::EntityRegistry& registry) const;
};

}  // namespace ecs::systems

