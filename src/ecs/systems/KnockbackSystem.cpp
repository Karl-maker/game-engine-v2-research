#include "ecs/systems/KnockbackSystem.h"

#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/events/CombatEvents.h"

#include <algorithm>

namespace ecs::systems {

void KnockbackSystem::tick(EntityRegistry& registry, ecs::services::EventService& events) const {
  const auto knockbacks = events.consumeAll<ecs::events::KnockbackEvent>();
  for (const auto& event : knockbacks) {
    auto* motion = registry.tryGet<ecs::MotionComponent>(event.targetEntity);
    if (!motion) continue;
    const auto* body = registry.tryGet<ecs::RigidbodyComponent>(event.targetEntity);
    if (body && body->kinematic) continue;

    math::Vec3 direction = math::normalize(event.direction);
    if (math::lengthSq(direction) <= 1e-6f) direction = {0.0f, 0.0f, 1.0f};
    const float supportScale = event.hadRaycastSupport ? 1.1f : 1.0f;
    const float collisionScale = event.hadPhysicsCollision ? 1.05f : 0.9f;
    const float impulse = std::max(0.0f, event.strength) * supportScale * collisionScale;
    motion->velocity = motion->velocity + direction * impulse + math::Vec3{0.0f, impulse * 0.08f, 0.0f};
  }
}

}  // namespace ecs::systems
