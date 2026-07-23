#include "ecs/systems/SocketSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/IdentityComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/SocketComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/TransformComponent.h"
#include "math/Mat4.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace {

static float degToRad(float degrees) { return degrees * 3.14159265358979323846f / 180.0f; }
static float radToDeg(float radians) { return radians * 180.0f / 3.14159265358979323846f; }

static math::Mat4 composeLocal(const ecs::SocketComponent& socket) {
  const math::Vec3 scale{1.0f + socket.scaleOffset.x, 1.0f + socket.scaleOffset.y, 1.0f + socket.scaleOffset.z};
  return math::mul(math::mul(math::mul(math::translate(socket.positionOffset), math::rotateZ(degToRad(socket.rotationOffset.z))),
                             math::rotateY(degToRad(socket.rotationOffset.y))),
                   math::mul(math::rotateX(degToRad(socket.rotationOffset.x)), math::scale(scale)));
}

static math::Mat4 composeWorld(const ecs::TransformComponent& tr) {
  return math::mul(math::translate(tr.position),
                   math::mul(math::rotateY(degToRad(tr.rotation.y)),
                             math::mul(math::rotateX(degToRad(tr.rotation.x)),
                                       math::mul(math::rotateZ(degToRad(tr.rotation.z)), math::scale(tr.scale)))));
}

static math::Vec3 scaleFromMat4(const math::Mat4& m) {
  const math::Vec3 x{m.m[0], m.m[1], m.m[2]};
  const math::Vec3 y{m.m[4], m.m[5], m.m[6]};
  const math::Vec3 z{m.m[8], m.m[9], m.m[10]};
  return {std::max(0.0001f, math::length(x)), std::max(0.0001f, math::length(y)), std::max(0.0001f, math::length(z))};
}

static math::Vec3 rotationFromMat4(const math::Mat4& m) {
  // Matches the engine's compose order: T * Ry * Rx * Rz * S.
  const math::Vec3 scale = scaleFromMat4(m);
  math::Mat4 normalized = m;
  normalized.m[0] /= scale.x;
  normalized.m[1] /= scale.x;
  normalized.m[2] /= scale.x;
  normalized.m[4] /= scale.y;
  normalized.m[5] /= scale.y;
  normalized.m[6] /= scale.y;
  normalized.m[8] /= scale.z;
  normalized.m[9] /= scale.z;
  normalized.m[10] /= scale.z;

  const float pitch = std::asin(std::clamp(-normalized.m[6], -1.0f, 1.0f));
  const float yaw = std::atan2(-normalized.m[8], normalized.m[10]);
  const float roll = std::atan2(normalized.m[1], normalized.m[5]);
  return {radToDeg(pitch), radToDeg(yaw), radToDeg(roll)};
}

static math::Mat4 boneWorldMatrix(const ecs::SkeletonComponent& skeleton, int boneIndex) {
  if (boneIndex < 0 || boneIndex >= static_cast<int>(skeleton.bones.size())) {
    return math::identity();
  }

  math::Mat4 local = math::identity();
  if (boneIndex < static_cast<int>(skeleton.currentPose.size())) {
    local = skeleton.currentPose[static_cast<std::size_t>(boneIndex)];
  } else if (boneIndex < static_cast<int>(skeleton.bindPose.size())) {
    local = skeleton.bindPose[static_cast<std::size_t>(boneIndex)];
  } else {
    local = skeleton.bones[static_cast<std::size_t>(boneIndex)].localBindTransform;
  }

  const int parent = skeleton.bones[static_cast<std::size_t>(boneIndex)].parentIndex;
  if (parent < 0 || skeleton.space == ecs::SkeletonComponent::Space::World) {
    return local;
  }
  return math::mul(boneWorldMatrix(skeleton, parent), local);
}

static int findBoneIndex(const ecs::SkeletonComponent& skeleton, const std::string& boneName) {
  int prefixMatch = -1;
  for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
    if (skeleton.bones[i].key == boneName) return static_cast<int>(i);
    if (prefixMatch < 0 && !boneName.empty() && skeleton.bones[i].key.rfind(boneName, 0) == 0) {
      prefixMatch = static_cast<int>(i);
    }
  }
  return prefixMatch;
}

static ecs::EntityId resolveEntityByName(ecs::EntityRegistry& registry, const std::string& name) {
  ecs::EntityId found = ecs::kInvalidEntityId;
  registry.view<ecs::IdentityComponent>([&](ecs::EntityId id, const ecs::IdentityComponent& identity) {
    if (found != ecs::kInvalidEntityId) return;
    if (identity.name == name) {
      found = id;
    }
  });
  return found;
}

static math::Vec3 translationFromMat4(const math::Mat4& m) { return {m.m[12], m.m[13], m.m[14]}; }

}  // namespace

namespace ecs::systems {

void SocketSystem::tick(EntityRegistry& registry, double deltaSeconds) const {
  static float debugTimerSeconds = 0.0f;
  debugTimerSeconds += static_cast<float>(std::max(0.0, deltaSeconds));

  registry.view<ecs::SocketComponent, ecs::TransformComponent>([&](ecs::EntityId socketId,
                                                                   ecs::SocketComponent& socket,
                                                                   ecs::TransformComponent& socketTr) {
    (void)socketId;
    if (!socket.enabled) return;

    ecs::EntityId targetEntity = socket.targetEntity;
    if (targetEntity == ecs::kInvalidEntityId && !socket.targetEntityName.empty()) {
      targetEntity = resolveEntityByName(registry, socket.targetEntityName);
    }
    if (!registry.isAlive(targetEntity)) return;

    const auto* targetTr = registry.tryGet<ecs::TransformComponent>(targetEntity);
    if (!targetTr) return;

    const auto* skeleton = registry.tryGet<ecs::SkeletonComponent>(targetEntity);
    if (!skeleton || !skeleton->enabled) return;
    if (!socket.skeletonName.empty() && skeleton->skeletonId != socket.skeletonName) {
      const auto* mesh = registry.tryGet<ecs::MeshComponent>(targetEntity);
      if (!mesh || mesh->skeletonId != socket.skeletonName) return;
    }

    const int boneIndex = findBoneIndex(*skeleton, socket.boneName);

    const math::Mat4 targetWorld = composeWorld(*targetTr);
    const math::Mat4 boneModel = boneIndex >= 0 ? boneWorldMatrix(*skeleton, boneIndex) : math::identity();
    socket.worldTransform = math::mul(targetWorld, math::mul(boneModel, composeLocal(socket)));
    socketTr.position = translationFromMat4(socket.worldTransform);
    socketTr.rotation = rotationFromMat4(socket.worldTransform);
    socketTr.scale = {1.0f, 1.0f, 1.0f};

    if (socket.name == "player_head_socket" && debugTimerSeconds >= 0.5f) {
      debugTimerSeconds = 0.0f;
      const math::Vec3 bonePos = translationFromMat4(math::mul(targetWorld, boneModel));
      const math::Vec3 socketPos = translationFromMat4(socket.worldTransform);
      std::cout << std::fixed << std::setprecision(3)
                << "[socket] bone=(" << bonePos.x << "," << bonePos.y << "," << bonePos.z << ")"
                << " socket=(" << socketPos.x << "," << socketPos.y << "," << socketPos.z << ")"
                << " delta=(" << (socketPos.x - bonePos.x) << "," << (socketPos.y - bonePos.y) << ","
                << (socketPos.z - bonePos.z) << ")\n";
    }
  });
}

}  // namespace ecs::systems
