#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"

namespace ecs::systems {

class IKSystem final {
 public:
  void tick(EntityRegistry& registry, ecs::services::EventService& events, double deltaSeconds) const;
};

}  // namespace ecs::systems
