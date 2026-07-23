#include "ecs/systems/ControllerSystem.h"

// Author: Karl-Johan Bailey

#include "core/ControlService.h"
#include "ecs/components/ControllerComponent.h"
#include "math/Vec3.h"

namespace ecs::systems {

void ControllerSystem::tick(ecs::EntityRegistry& registry, const core::ControlService& controls) const {
  const auto& state = controls.state();

  registry.view<ecs::ControllerComponent>([&](ecs::EntityId, ecs::ControllerComponent& c) {
    if (!c.enabled) return;
    if (c.mode != ecs::ControllerComponent::Mode::Player) return;

    c.actionRequests.clear();
    c.moveRequest.hasRequest = true;
    c.moveRequest.hasDirection = true;
    c.moveRequest.direction = state.moveDirection;
    if (state.crouch) {
      c.moveRequest.moveMode = ecs::ControllerComponent::MoveMode::Crouch;
    } else if (state.sprint) {
      c.moveRequest.moveMode = ecs::ControllerComponent::MoveMode::Sprint;
    } else {
      c.moveRequest.moveMode = ecs::ControllerComponent::MoveMode::Walk;
    }

    const bool hasLook = (state.lookDeltaDeg.x != 0.0f) || (state.lookDeltaDeg.y != 0.0f) || (state.lookDeltaDeg.z != 0.0f);
    c.lookRequest.hasRequest = hasLook;
    c.lookRequest.lookDelta = state.lookDeltaDeg;

    if (state.jump) {
      c.actionRequests.push_back({"jump", true, 1.0f});
    }
  });
}

}  // namespace ecs::systems
