#include "ecs/systems/GravitySystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"

#include <algorithm>

namespace ecs::systems {

void GravitySystem::tick(EntityRegistry& registry, double deltaSeconds) const {
  const float dt = static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.25));
  if (dt <= 0.0f || strength <= 0.0f) return;

  const math::Vec3 g = math::normalize(direction) * (strength * dt);
  registry.view<ecs::MotionComponent, ecs::RigidbodyComponent>(
      [&](ecs::EntityId, ecs::MotionComponent& motion, const ecs::RigidbodyComponent& body) {
        if (body.kinematic || !body.useGravity) return;
        if (!motion.isGrounded) {
          motion.velocity = motion.velocity + g;
        }
      });
}

}  // namespace ecs::systems
