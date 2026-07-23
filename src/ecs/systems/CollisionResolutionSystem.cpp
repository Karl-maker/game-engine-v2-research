#include "ecs/systems/CollisionResolutionSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/CollisionEvents.h"
#include "math/Vec3.h"

#include <algorithm>

namespace {

float inverseMassFor(const ecs::RigidbodyComponent* body, const ecs::MotionComponent* motion) {
  if (!motion) return 0.0f;
  if (!body) return 1.0f;
  if (body->kinematic || body->mass <= 0.0f) return 0.0f;
  return body->inverseMass > 0.0f ? body->inverseMass : (1.0f / body->mass);
}

void removeClosingVelocity(ecs::MotionComponent& motion, const math::Vec3& normal) {
  const float intoSurface = math::dot(motion.velocity, normal);
  if (intoSurface < 0.0f) {
    motion.velocity = motion.velocity - normal * intoSurface;
  }
}

}  // namespace

namespace ecs::systems {

void CollisionResolutionSystem::tick(EntityRegistry& registry, ecs::services::EventService& events) const {
  const auto contacts = events.consumeAll<ecs::events::CollisionDetectionEvent>();

  registry.view<ecs::MotionComponent>([](ecs::EntityId, ecs::MotionComponent& motion) { motion.isGrounded = false; });

  for (const auto& c : contacts) {
    auto* trA = registry.tryGet<ecs::TransformComponent>(c.entityA);
    auto* trB = registry.tryGet<ecs::TransformComponent>(c.entityB);
    auto* motionA = registry.tryGet<ecs::MotionComponent>(c.entityA);
    auto* motionB = registry.tryGet<ecs::MotionComponent>(c.entityB);
    if ((!trA || !motionA) && (!trB || !motionB)) continue;

    const ecs::RigidbodyComponent* bodyA = registry.tryGet<ecs::RigidbodyComponent>(c.entityA);
    const ecs::RigidbodyComponent* bodyB = registry.tryGet<ecs::RigidbodyComponent>(c.entityB);
    const float invA = inverseMassFor(bodyA, motionA);
    const float invB = inverseMassFor(bodyB, motionB);
    const float totalInv = invA + invB;
    if (totalInv <= 0.0f) continue;

    const float slop = 0.003f;
    const float depth = std::max(0.0f, c.penetrationDepth - slop);
    const math::Vec3 n = math::normalize(c.contactNormal);
    const math::Vec3 totalCorrection = n * depth;
    const math::Vec3 correctionA = totalCorrection * (invA / totalInv);
    const math::Vec3 correctionB = totalCorrection * (-invB / totalInv);

    if (trA && motionA && invA > 0.0f) {
      trA->position = trA->position + correctionA;
      removeClosingVelocity(*motionA, n);
      if (n.y > 0.55f) motionA->isGrounded = true;
    }
    if (trB && motionB && invB > 0.0f) {
      trB->position = trB->position + correctionB;
      removeClosingVelocity(*motionB, n * -1.0f);
      if (n.y < -0.55f) motionB->isGrounded = true;
    }

    events.emit<ecs::events::CollisionResolutionEvent>(
        {c.entityA, c.entityB, correctionA, correctionB, n, c.penetrationDepth, c.time});
  }
}

}  // namespace ecs::systems
