#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"

namespace ecs::services {

struct WeaponConfig {
  // Character creation configuration goes here.
};

class WeaponFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const WeaponConfig& config);
};

}  // namespace ecs::services