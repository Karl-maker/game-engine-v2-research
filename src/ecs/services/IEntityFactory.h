#pragma once

// Author: Karl-Johan Bailey
//
// IEntityFactory
// Data-driven entity factory interface. Factories create ECS entities based on JSON config.

#include "data/Json.h"
#include "ecs/EntityId.h"
#include "math/Vec3.h"

namespace ecs {
class EntityRegistry;
}

namespace ecs::services {

struct FactoryContext final {
  math::Vec3 chunkOriginWorld{0.0f, 0.0f, 0.0f};
};

class IEntityFactory {
 public:
  virtual ~IEntityFactory() = default;
  virtual EntityId create(EntityRegistry& registry, const data::JsonValue& config, const FactoryContext& ctx) = 0;
};

}  // namespace ecs::services

