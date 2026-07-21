#include "ecs/systems/TargetSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/TargetComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/systems/Angle.h"

#include <algorithm>
#include <cmath>

namespace ecs::systems {

static float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }

static void computeYawPitchDeg(const math::Vec3& dir, float& outYawDeg, float& outPitchDeg) {
  // Convention:
  // - yaw: rotation around Y, looking down +Z, positive yaw turns toward +X
  // - pitch: up/down
  const float yaw = std::atan2(dir.x, dir.z);
  const float xz = std::sqrt(dir.x * dir.x + dir.z * dir.z);
  const float pitch = std::atan2(dir.y, xz);

  outYawDeg = radToDeg(yaw);
  outPitchDeg = radToDeg(pitch);
}

void TargetSystem::update(EntityRegistry& registry, double deltaSeconds) {
  const float dt = static_cast<float>(deltaSeconds);

  registry.view<TransformComponent, TargetComponent>(
      [&](EntityId id, TransformComponent& self, TargetComponent& tc) {
        (void)id;
        if (!tc.enabled) return;
        if (!registry.isAlive(tc.targetEntity)) return;

        const auto* targetTr = registry.tryGet<TransformComponent>(tc.targetEntity);
        if (!targetTr) return;

        const math::Vec3 targetPos = targetTr->position + tc.targetOffset;
        const math::Vec3 dir = targetPos - self.position;
        if (math::lengthSq(dir) < 0.000001f) return;

        float desiredYaw = 0.0f;
        float desiredPitch = 0.0f;
        computeYawPitchDeg(dir, desiredYaw, desiredPitch);

        const float maxDelta = std::max(0.0f, tc.rotationSpeed) * dt;
        self.rotation.x = moveTowardsAngleDegrees(self.rotation.x, desiredPitch, maxDelta);  // pitch
        self.rotation.y = moveTowardsAngleDegrees(self.rotation.y, desiredYaw, maxDelta);    // yaw

        if (tc.lockRoll) {
          self.rotation.z = 0.0f;
        }

        // upVector is included for completeness; a full look-at would build a basis from it.
        (void)clamp01;
      });
}

}  // namespace ecs::systems

