#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct ObjectConfig final {
  TransformInput transform{.name = "object"};
  ViewableInput viewable{};
};

class ObjectFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const ObjectConfig& config);
};

}  // namespace ecs::services
