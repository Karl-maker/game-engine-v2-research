#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class HierarchySystem final {
 public:
  void tick(EntityRegistry& registry) const;
};

}  // namespace ecs::systems
