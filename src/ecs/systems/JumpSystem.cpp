#include "ecs/systems/JumpSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ControllerComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"

namespace ecs::systems {

void JumpSystem::tick(EntityRegistry& registry) const {
  registry.view<ecs::ControllerComponent, ecs::MotionComponent>(
      [&](ecs::EntityId id, const ecs::ControllerComponent& controller, ecs::MotionComponent& motion) {
        if (!controller.enabled || !motion.isGrounded) return;

        bool jumpPressed = false;
        for (const auto& action : controller.actionRequests) {
          if (action.pressed && action.action == "jump") {
            jumpPressed = true;
            break;
          }
        }
        if (!jumpPressed) return;

        motion.velocity.y = jumpSpeed;
        motion.isGrounded = false;
        motion.mode = ecs::MotionComponent::Mode::Jumping;
        motion.movementPhase = ecs::MotionComponent::MovementPhase::Starting;

        if (auto* body = registry.tryGet<ecs::RigidbodyComponent>(id)) {
          body->linearVelocity.y = jumpSpeed;
        }
      });
}

}  // namespace ecs::systems
