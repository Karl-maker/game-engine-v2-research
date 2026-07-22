#pragma once

// Author: Karl-Johan Bailey
//
// MovementSystem
// Integrates motion intent into velocity and applies movement to transforms.
// Emits motion-related events when state changes.

#include "ecs/EntityRegistry.h"

namespace ecs::services {
class EventService;
}

namespace ecs::systems {

class MovementSystem final {
 public:
  void tick(ecs::EntityRegistry& registry, ecs::services::EventService& events, double deltaSeconds) const;
};

}  // namespace ecs::systems

