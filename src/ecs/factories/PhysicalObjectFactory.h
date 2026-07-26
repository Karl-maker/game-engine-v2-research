#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct PhysicalObjectConfig final {
  TransformInput transform{.name = "physical_object"};
  ViewableInput viewable{};
  PhysicalInput physical{};
};

class PhysicalObjectFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const PhysicalObjectConfig& config);
};

}  // namespace ecs::services
