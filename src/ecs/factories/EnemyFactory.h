#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"

namespace ecs::services {

struct EnemyConfig {
  // Character creation configuration goes here.
};

class EnemyFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const EnemyConfig& config);
};

}  // namespace ecs::services