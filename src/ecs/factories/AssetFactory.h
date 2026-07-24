#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"

namespace ecs::services {

struct AssetConfig {
  // Character creation configuration goes here.
};

class AssetFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const AssetConfig& config);
};

}  // namespace ecs::services