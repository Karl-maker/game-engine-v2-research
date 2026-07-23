#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"

namespace ecs::systems {

class SensorSystem final {
 public:
  void tick(EntityRegistry& registry, ecs::services::EventService& events) const;
};

}  // namespace ecs::systems
