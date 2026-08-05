#pragma once

// Author: Karl-Johan Bailey
//
// BillboardFactory
// Spawns a world-space textured quad, optionally animated by cycling image frames.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct BillboardConfig final {
  BillboardInput billboard{};
};

class BillboardFactory final {
 public:
  EntityId create(EntityRegistry& registry, const BillboardConfig& config);
};

}  // namespace ecs::services
