#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"

namespace ecs::services {

struct BuildingConfig {
  // Character creation configuration goes here.
};

class BuildingFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const BuildingConfig& config);
};

}  // namespace ecs::services