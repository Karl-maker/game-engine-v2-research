#include "ecs/systems/MotionSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ControllerComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/TransformComponent.h"
#include "math/Vec3.h"

#include <cmath>

namespace {

static math::Vec3 cross(const math::Vec3& a, const math::Vec3& b) {
  return math::Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static math::Vec3 forwardFromPitchYawDeg(float pitchDeg, float yawDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kDegToRad = kPi / 180.0f;

  const float pitch = pitchDeg * kDegToRad;
  const float yaw = yawDeg * kDegToRad;

  // Matches GraphicsSystem camera convention:
  // - yaw=0, pitch=0 => +Z forward
  // - positive yaw turns toward +X
  // - positive pitch looks downward, hence -sin(pitch) on Y
  const math::Vec3 fwd{
      std::cos(pitch) * std::sin(yaw),
      -std::sin(pitch),
      std::cos(pitch) * std::cos(yaw),
  };

  return math::normalize(fwd);
}

static math::Vec3 toMoveDirectionFromView(const math::Vec3& local, const ecs::TransformComponent& tr, bool flying) {
  const math::Vec3 up{0.0f, 1.0f, 0.0f};
  math::Vec3 fwd = forwardFromPitchYawDeg(tr.rotation.x, tr.rotation.y);
  math::Vec3 right = cross(fwd, up);
  if (math::lengthSq(right) < 1e-6f) {
    // Degenerate (looking straight up/down); fall back to yaw-only right.
    right = forwardFromPitchYawDeg(0.0f, tr.rotation.y);
    right = cross(right, up);
  }
  right = math::normalize(right);

  if (!flying) {
    fwd.y = 0.0f;
    right.y = 0.0f;
    fwd = math::normalize(fwd);
    right = math::normalize(right);
  }

  return fwd * local.z + right * local.x;
}

}  // namespace

namespace ecs::systems {

namespace {

static float speedFor(const ecs::ControllerComponent& controller, const ecs::StatsComponent* stats) {
  const bool hasWalk = stats && stats->walkingSpeed > 0.0f;
  const bool hasRun = stats && stats->runningSpeed > 0.0f;

  switch (controller.moveRequest.moveMode) {
    case ecs::ControllerComponent::MoveMode::Sprint:
      return hasRun ? stats->runningSpeed : 10.0f;
    case ecs::ControllerComponent::MoveMode::Crouch:
      return (hasWalk ? stats->walkingSpeed : 6.0f) * 0.5f;
    case ecs::ControllerComponent::MoveMode::Walk:
    default:
      return hasWalk ? stats->walkingSpeed : 6.0f;
  }
}

}  // namespace

void MotionSystem::tick(ecs::EntityRegistry& registry) const {
  registry.view<ecs::ControllerComponent, ecs::MotionComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::ControllerComponent& controller,
          ecs::MotionComponent& motion,
          const ecs::TransformComponent& tr) {
    if (!controller.enabled) return;

    const ecs::StatsComponent* stats = registry.tryGet<ecs::StatsComponent>(id);

    math::Vec3 dir{0.0f, 0.0f, 0.0f};
    if (controller.moveRequest.hasRequest && controller.moveRequest.hasDirection) {
      dir = controller.moveRequest.direction;
    }

    if (math::lengthSq(dir) > 1e-6f) {
      dir = math::normalize(dir);
      const bool flying = motion.mode == ecs::MotionComponent::Mode::Flying;
      const math::Vec3 worldDir = math::normalize(toMoveDirectionFromView(dir, tr, flying));
      motion.desiredDirection = worldDir;
      const float speed = speedFor(controller, stats);
      motion.desiredVelocity = worldDir * speed;
    } else {
      motion.desiredDirection = {0.0f, 0.0f, 0.0f};
      motion.desiredVelocity = {0.0f, 0.0f, 0.0f};
    }
  });
}

}  // namespace ecs::systems
