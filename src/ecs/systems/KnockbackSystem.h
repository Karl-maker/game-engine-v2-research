#pragma once

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"

namespace ecs::systems {

class KnockbackSystem final {
 public:
  void tick(EntityRegistry& registry, ecs::services::EventService& events) const;
};

}  // namespace ecs::systems
