#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct GrassPatchConfig final {
  TransformInput transform{.name = "object"};
  GrassInput grass{};
};

class GrassPatchFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const GrassPatchConfig& config);
};

}  // namespace ecs::services
