#pragma once

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"

namespace ecs::systems {

class CombatInteractionSystem final {
 public:
  void tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) const;
};

}  // namespace ecs::systems
