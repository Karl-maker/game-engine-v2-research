#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"

namespace ecs::systems {

class InteractionDetectionSystem final {
 public:

  void tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) const;
};

}  // namespace ecs::systems
