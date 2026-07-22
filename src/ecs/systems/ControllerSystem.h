#pragma once

// Author: Karl-Johan Bailey
//
// ControllerSystem (demo-focused)
// Applies the current ControlService state to entities with ControllerComponent.
// This keeps the ControllerComponent as the single "request" surface for intent.

#include "ecs/EntityRegistry.h"

namespace core {
class ControlService;
}

namespace ecs::systems {

class ControllerSystem final {
 public:
  void tick(ecs::EntityRegistry& registry, const core::ControlService& controls) const;
};

}  // namespace ecs::systems

