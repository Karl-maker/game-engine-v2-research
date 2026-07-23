#include "ecs/systems/ThirdPersonCameraSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ControllerComponent.h"
#include "ecs/components/ThirdPersonCameraComponent.h"
#include "ecs/components/TransformComponent.h"
#include "math/Vec3.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;
constexpr float kRadToDeg = 180.0f / kPi;

math::Vec3 forwardFromPitchYawDeg(float pitchDeg, float yawDeg) {
  const float pitch = pitchDeg * kDegToRad;
  const float yaw = yawDeg * kDegToRad;
  return math::normalize(math::Vec3{std::cos(pitch) * std::sin(yaw), -std::sin(pitch), std::cos(pitch) * std::cos(yaw)});
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

math::Vec3 lerpVec3(const math::Vec3& a, const math::Vec3& b, float t) {
  return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}

}  // namespace

namespace ecs::systems {

void ThirdPersonCameraSystem::tick(EntityRegistry& registry, double deltaSeconds) const {
  const float dt = static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.25));

  registry.view<ecs::ThirdPersonCameraComponent, ecs::TransformComponent>(
      [&](ecs::EntityId, ecs::ThirdPersonCameraComponent& cam, ecs::TransformComponent& camTr) {
        if (!cam.enabled || cam.target == ecs::kInvalidEntityId) return;
        auto* targetTr = registry.tryGet<ecs::TransformComponent>(cam.target);
        if (!targetTr) return;

        if (auto* controller = registry.tryGet<ecs::ControllerComponent>(cam.target)) {
          if (controller->enabled && controller->lookRequest.hasRequest) {
            cam.yawDeg += controller->lookRequest.lookDelta.y;
            cam.pitchDeg = std::clamp(cam.pitchDeg + controller->lookRequest.lookDelta.x, cam.minPitchDeg, cam.maxPitchDeg);
            targetTr->rotation.y = cam.yawDeg;
          }
        }

        const math::Vec3 target = targetTr->position + cam.targetOffset;
        const math::Vec3 fwd = forwardFromPitchYawDeg(cam.pitchDeg, cam.yawDeg);
        const math::Vec3 desired = target - fwd * cam.distance + math::Vec3{0.0f, cam.height, 0.0f};
        const float t = 1.0f - std::exp(-cam.followSharpness * dt);
        camTr.position = lerpVec3(camTr.position, desired, t);

        const math::Vec3 toTarget = math::normalize(target - camTr.position);
        camTr.rotation.x = std::asin(std::clamp(-toTarget.y, -1.0f, 1.0f)) * kRadToDeg;
        camTr.rotation.y = std::atan2(toTarget.x, toTarget.z) * kRadToDeg;
        camTr.rotation.z = 0.0f;
      });
}

}  // namespace ecs::systems
