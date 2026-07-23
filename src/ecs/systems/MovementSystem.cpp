#include "ecs/systems/MovementSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ControllerComponent.h"
#include "ecs/components/CharacterComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/MotionEvents.h"
#include "ecs/services/EventService.h"
#include "math/Vec3.h"

#include <algorithm>
#include <cmath>

namespace ecs::systems {

namespace {

static math::Vec3 clampDelta(const math::Vec3& delta, float maxLen) {
  const float lenSq = math::lengthSq(delta);
  if (lenSq <= maxLen * maxLen) return delta;
  const float invLen = 1.0f / std::max(0.000001f, std::sqrt(lenSq));
  return delta * (maxLen * invLen);
}

static float accelFor(const ecs::StatsComponent* stats) {
  // Simple heuristic: allow higher accel for faster actors.
  if (!stats) return 24.0f;
  const float base = std::max({stats->walkingSpeed, stats->runningSpeed, 6.0f});
  return std::clamp(base * 4.0f, 18.0f, 80.0f);
}

}  // namespace

void MovementSystem::tick(ecs::EntityRegistry& registry, ecs::services::EventService& events, double deltaSeconds) const {
  const float dt = static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.25));
  if (dt <= 0.0f) return;

  registry.view<ecs::TransformComponent, ecs::MotionComponent>(
      [&](ecs::EntityId id, ecs::TransformComponent& tr, ecs::MotionComponent& motion) {
        const bool wasMoving = motion.isMoving;
        const bool wasGrounded = motion.isGrounded;

        // Optional look intent (apply here so rotation is updated once, centralized with movement).
        if (auto* controller = registry.tryGet<ecs::ControllerComponent>(id)) {
          if (controller->enabled && controller->lookRequest.hasRequest) {
            const bool characterDriven = registry.has<ecs::CharacterComponent>(id);
            if (characterDriven) {
              tr.rotation.z += controller->lookRequest.lookDelta.z;
            } else {
              tr.rotation.x += controller->lookRequest.lookDelta.x;
              tr.rotation.y += controller->lookRequest.lookDelta.y;
              tr.rotation.z += controller->lookRequest.lookDelta.z;
            }
          }
        }

        math::Vec3 desiredVel = motion.desiredVelocity;

        // Non-flying actors keep vertical velocity under gravity/collision/jump control.
        if (motion.mode != ecs::MotionComponent::Mode::Flying) {
          desiredVel.y = motion.velocity.y;
        }

        const ecs::StatsComponent* stats = registry.tryGet<ecs::StatsComponent>(id);
        const float accel = accelFor(stats);
        const float maxDeltaV = accel * dt;

        const math::Vec3 dv = desiredVel - motion.velocity;
        const math::Vec3 appliedDv = clampDelta(dv, maxDeltaV);
        motion.velocity = motion.velocity + appliedDv;
        motion.acceleration = appliedDv * (1.0f / dt);

        tr.position = tr.position + motion.velocity * dt;

        motion.currentSpeed = math::length(motion.velocity);
        motion.isMoving = motion.currentSpeed > 0.05f;

        if (!wasMoving && motion.isMoving) {
          events.emit<ecs::events::MotionStartedEvent>({id});
          motion.movementPhase = ecs::MotionComponent::MovementPhase::Starting;
        } else if (wasMoving && !motion.isMoving) {
          events.emit<ecs::events::MotionStoppedEvent>({id});
          motion.movementPhase = ecs::MotionComponent::MovementPhase::Stopping;
        } else if (motion.isMoving) {
          motion.movementPhase = ecs::MotionComponent::MovementPhase::Moving;
        } else {
          motion.movementPhase = ecs::MotionComponent::MovementPhase::Idle;
        }

        if (!wasGrounded && motion.isGrounded) {
          events.emit<ecs::events::LandedEvent>({id});
        }
      });
}

}  // namespace ecs::systems
