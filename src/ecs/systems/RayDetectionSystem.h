#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"
#include "ecs/services/SpatialHashGridService.h"

namespace ecs::systems {

class RayDetectionSystem final {
 public:
  void tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds);

 private:
  ecs::services::SpatialHashGridService m_grid{4.0f};
};

}  // namespace ecs::systems
