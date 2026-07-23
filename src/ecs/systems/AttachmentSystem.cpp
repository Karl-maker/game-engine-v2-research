#include "ecs/systems/AttachmentSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/SocketComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/systems/Angle.h"

#include <algorithm>
#include <cmath>

namespace ecs::systems {

static float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }

static math::Vec3 rotateYawPitch(const math::Vec3& v, float yawDeg, float pitchDeg) {
  const float yaw = degToRad(yawDeg);
  const float pitch = degToRad(pitchDeg);

  // Yaw around Y.
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  math::Vec3 yawed{v.x * cy + v.z * sy, v.y, -v.x * sy + v.z * cy};

  // Pitch around X.
  const float cp = std::cos(pitch);
  const float sp = std::sin(pitch);
  return {yawed.x, yawed.y * cp - yawed.z * sp, yawed.y * sp + yawed.z * cp};
}

static void applyFollow(TransformComponent& self,
                        const TransformComponent& target,
                        const AttachmentComponent::Attachment& a,
                        float dt) {
  const math::Vec3 desiredPos = target.position + a.positionOffset;
  const float alpha = clamp01(static_cast<float>(a.positionSpeed) * dt);
  self.position = math::lerp(self.position, desiredPos, alpha);

  if (a.inheritRotation) {
    self.rotation = math::lerp(self.rotation, target.rotation + a.rotationOffset, clamp01(a.rotationSpeed * dt));
  }
}

static void applyParent(TransformComponent& self,
                        const TransformComponent& target,
                        const AttachmentComponent::Attachment& a) {
  self.position = target.position + a.positionOffset;
  if (a.inheritRotation) self.rotation = target.rotation + a.rotationOffset;
  if (a.inheritScale) self.scale = target.scale;  // scaleOffset left to a real system
}

static void applyOrbit(TransformComponent& self,
                       const TransformComponent& target,
                       AttachmentComponent::Attachment& a,
                       float dt) {
  (void)dt;
  a.currentPitchDeg = std::max(a.minPitchDeg, std::min(a.maxPitchDeg, a.currentPitchDeg));

  // Orbit offset starts behind target along -Z.
  const math::Vec3 base{0.0f, 0.0f, -a.orbitRadius};
  const math::Vec3 orbitOffset = rotateYawPitch(base, a.currentYawDeg, a.currentPitchDeg);
  self.position = target.position + orbitOffset + a.positionOffset;
}

static bool socketMatchesTarget(const ecs::SocketComponent& socket, ecs::EntityId targetId, const ecs::IdentityComponent* targetIdentity) {
  if (socket.targetEntity != ecs::kInvalidEntityId) {
    return socket.targetEntity == targetId;
  }
  if (socket.targetEntityName.empty() || !targetIdentity) return false;
  return socket.targetEntityName == targetIdentity->name;
}

void AttachmentSystem::update(EntityRegistry& registry, double deltaSeconds) {
  const float dt = static_cast<float>(deltaSeconds);

  registry.view<TransformComponent, AttachmentComponent>(
      [&](EntityId id, TransformComponent& self, AttachmentComponent& ac) {
        (void)id;
        for (auto& a : ac.attachments) {
          if (!a.enabled) continue;
          if (!registry.isAlive(a.targetEntity)) continue;

          const auto* targetTr = registry.tryGet<TransformComponent>(a.targetEntity);
          if (!targetTr) continue;
          const auto* targetIdentity = registry.tryGet<IdentityComponent>(a.targetEntity);

          // Local vs world space support is system-defined; this demo treats offsets as world-space.
          // In a full engine, you would interpret `a.space` + inheritance flags properly.
          switch (a.mode) {
            case AttachmentComponent::Mode::Parent:
              applyParent(self, *targetTr, a);
              break;
            case AttachmentComponent::Mode::Follow:
            case AttachmentComponent::Mode::Spring:
              applyFollow(self, *targetTr, a, dt);
              break;
            case AttachmentComponent::Mode::Socket: {
              bool foundSocket = false;
              registry.view<SocketComponent, TransformComponent>([&](ecs::EntityId, const SocketComponent& socket, const TransformComponent& socketTr) {
                if (foundSocket || !socket.enabled) return;
                if (socket.name != a.socketKey) return;
                if (!socketMatchesTarget(socket, a.targetEntity, targetIdentity)) return;
                self = socketTr;
                foundSocket = true;
              });
              if (!foundSocket) {
                applyFollow(self, *targetTr, a, dt);
              }
              break;
            }
            case AttachmentComponent::Mode::Orbit:
              applyOrbit(self, *targetTr, a, dt);
              break;
            case AttachmentComponent::Mode::LookAt:
              // LookAt is covered by TargetComponent/TargetSystem in this project.
              // Keep this mode for data completeness.
              break;
          }
        }
      });
}

}  // namespace ecs::systems
