#include "ecs/systems/IKSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/IKComponent.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/IKEvents.h"
#include "math/Mat4.h"

#include <algorithm>
#include <cmath>

namespace {

static math::Vec3 translationFromMat4(const math::Mat4& m) {
  return {m.m[12], m.m[13], m.m[14]};
}

static math::Vec3 transformVector(const math::Mat4& m, const math::Vec3& v) {
  return {
      m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z,
      m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z,
      m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z,
  };
}

static ecs::EntityId resolveEntityByName(ecs::EntityRegistry& registry, const std::string& name) {
  ecs::EntityId found = ecs::kInvalidEntityId;
  registry.view<ecs::IdentityComponent>([&](ecs::EntityId id, const ecs::IdentityComponent& identity) {
    if (found != ecs::kInvalidEntityId) return;
    if (identity.name == name) found = id;
  });
  return found;
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

static math::Mat4 composeWorld(const ecs::TransformComponent& tr) {
  return math::mul(math::translate(tr.position),
                   math::mul(math::rotateY(tr.rotation.y * 3.14159265358979323846f / 180.0f),
                             math::mul(math::rotateX(tr.rotation.x * 3.14159265358979323846f / 180.0f),
                                       math::mul(math::rotateZ(tr.rotation.z * 3.14159265358979323846f / 180.0f),
                                                 math::scale(tr.scale)))));
}

static math::Vec3 scaleFromMat4(const math::Mat4& m) {
  const math::Vec3 x{m.m[0], m.m[1], m.m[2]};
  const math::Vec3 y{m.m[4], m.m[5], m.m[6]};
  const math::Vec3 z{m.m[8], m.m[9], m.m[10]};
  return {std::max(0.0001f, math::length(x)), std::max(0.0001f, math::length(y)), std::max(0.0001f, math::length(z))};
}

static math::Vec3 rotationFromMat4(const math::Mat4& m) {
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
  return {pitch, yaw, roll};
}

static math::Vec3 directionToRotation(const math::Vec3& dir) {
  const math::Vec3 normalized = math::normalize(dir);
  if (math::lengthSq(normalized) <= 0.000001f) return {0.0f, 0.0f, 0.0f};
  const float yaw = std::atan2(normalized.x, normalized.z);
  const float pitch = std::atan2(-normalized.y, std::sqrt(normalized.x * normalized.x + normalized.z * normalized.z));
  return {pitch, yaw, 0.0f};
}

static math::Vec3 rotateVector(const math::Vec3& v, const math::Vec3& rotationDeg) {
  const float rx = rotationDeg.x * 3.14159265358979323846f / 180.0f;
  const float ry = rotationDeg.y * 3.14159265358979323846f / 180.0f;
  const float rz = rotationDeg.z * 3.14159265358979323846f / 180.0f;

  const float cx = std::cos(rx);
  const float sx = std::sin(rx);
  const float cy = std::cos(ry);
  const float sy = std::sin(ry);
  const float cz = std::cos(rz);
  const float sz = std::sin(rz);

  math::Vec3 out = v;
  out = {out.x, out.y * cx - out.z * sx, out.y * sx + out.z * cx};
  out = {out.x * cy + out.z * sy, out.y, -out.x * sy + out.z * cy};
  out = {out.x * cz - out.y * sz, out.x * sz + out.y * cz, out.z};
  return out;
}

static math::Mat4 boneWorldMatrix(const ecs::SkeletonComponent& skeleton, int boneIndex) {
  if (boneIndex < 0 || boneIndex >= static_cast<int>(skeleton.bones.size())) {
    return math::identity();
  }
  math::Mat4 local = skeleton.currentPose.size() > static_cast<std::size_t>(boneIndex)
                         ? skeleton.currentPose[static_cast<std::size_t>(boneIndex)]
                         : skeleton.bones[static_cast<std::size_t>(boneIndex)].localBindTransform;
  const int parent = skeleton.bones[static_cast<std::size_t>(boneIndex)].parentIndex;
  if (parent < 0 || skeleton.space == ecs::SkeletonComponent::Space::World) {
    return local;
  }
  return math::mul(boneWorldMatrix(skeleton, parent), local);
}

static void collectChainIndices(const ecs::SkeletonComponent& skeleton,
                                const std::vector<std::string>& boneNames,
                                std::vector<int>& out) {
  out.clear();
  std::vector<int> seen;
  for (const auto& boneName : boneNames) {
    const int idx = findBoneIndex(skeleton, boneName);
    if (idx < 0) continue;
    if (std::find(seen.begin(), seen.end(), idx) != seen.end()) continue;
    out.push_back(idx);
    seen.push_back(idx);
  }
}

static std::vector<math::Vec3> collectWorldPositions(const ecs::SkeletonComponent& skeleton,
                                                     const ecs::TransformComponent& entityTr,
                                                     const std::vector<int>& chain) {
  std::vector<math::Vec3> positions;
  positions.reserve(chain.size());
  const math::Mat4 rootWorld = composeWorld(entityTr);
  for (int idx : chain) {
    const math::Mat4 world = math::mul(rootWorld, boneWorldMatrix(skeleton, idx));
    positions.push_back(translationFromMat4(world));
  }
  return positions;
}

static math::Mat4 composeLocal(const math::Vec3& translation, const math::Vec3& rotationRad, const math::Vec3& scale) {
  return math::mul(math::translate(translation),
                   math::mul(math::rotateY(rotationRad.y),
                             math::mul(math::rotateX(rotationRad.x),
                                       math::mul(math::rotateZ(rotationRad.z), math::scale(scale)))));
}

static math::Vec3 solveFabrik(const std::vector<math::Vec3>& starts,
                              const math::Vec3& target,
                              std::vector<math::Vec3>& outPositions,
                              int iterations) {
  outPositions = starts;
  if (outPositions.size() < 2) return target;

  const std::size_t count = outPositions.size();
  std::vector<float> lengths(count - 1, 0.0f);
  float totalLength = 0.0f;
  for (std::size_t i = 0; i + 1 < count; ++i) {
    lengths[i] = math::length(outPositions[i + 1] - outPositions[i]);
    totalLength += lengths[i];
  }

  const math::Vec3 root = outPositions.front();
  if (math::length(target - root) >= totalLength) {
    const math::Vec3 dir = math::normalize(target - root);
    outPositions.front() = root;
    for (std::size_t i = 1; i < count; ++i) {
      outPositions[i] = outPositions[i - 1] + dir * lengths[i - 1];
    }
    return outPositions.back();
  }

  for (int iter = 0; iter < std::max(1, iterations); ++iter) {
    outPositions.back() = target;
    for (std::size_t i = count - 1; i-- > 0;) {
      const math::Vec3 dir = math::normalize(outPositions[i] - outPositions[i + 1]);
      outPositions[i] = outPositions[i + 1] + dir * lengths[i];
    }

    outPositions.front() = root;
    for (std::size_t i = 0; i + 1 < count; ++i) {
      const math::Vec3 dir = math::normalize(outPositions[i + 1] - outPositions[i]);
      outPositions[i + 1] = outPositions[i] + dir * lengths[i];
    }
  }

  return outPositions.back();
}

}  // namespace

namespace ecs::systems {

void IKSystem::tick(EntityRegistry& registry, ecs::services::EventService& events) const {
  registry.view<ecs::IKComponent, ecs::SkeletonComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, ecs::IKComponent& ik, ecs::SkeletonComponent& skeleton, ecs::TransformComponent& tr) {
        if (!ik.enabled || !skeleton.enabled || skeleton.bones.empty()) return;

        ik.solvedBoneNames.clear();

        for (auto& chain : ik.chains) {
          if (!chain.enabled || !chain.overrideAnimation || chain.boneNames.empty()) continue;

          math::Vec3 target = chain.worldTarget;
          if (chain.targetMode == ecs::IKComponent::Chain::TargetMode::Entity) {
            ecs::EntityId targetEntity = chain.targetEntity;
            if (targetEntity == ecs::kInvalidEntityId && !chain.targetEntityName.empty()) {
              targetEntity = resolveEntityByName(registry, chain.targetEntityName);
            }

            target = tr.position + chain.targetOffset;
            if (registry.isAlive(targetEntity)) {
              if (const auto* targetTr = registry.tryGet<ecs::TransformComponent>(targetEntity)) {
                target = targetTr->position + chain.targetOffset + rotateVector(chain.targetLocalOffset, targetTr->rotation);
              }
            }
          }

          std::vector<int> chainIndices;
          collectChainIndices(skeleton, chain.boneNames, chainIndices);
          if (chainIndices.size() < 2) continue;

          auto startPositions = collectWorldPositions(skeleton, tr, chainIndices);
          std::vector<math::Vec3> solvedPositions;
          solveFabrik(startPositions, target, solvedPositions, chain.iterations);

          const math::Mat4 rootWorld = composeWorld(tr);
          math::Mat4 parentWorld = rootWorld;
          for (std::size_t i = 0; i < chainIndices.size(); ++i) {
            const int boneIndex = chainIndices[i];
            auto& pose = skeleton.currentPose[static_cast<std::size_t>(boneIndex)];
            const math::Mat4 inverseParent = math::inverseAffine(parentWorld);
            const math::Vec3 currentTranslation = translationFromMat4(pose);
            const math::Vec3 currentScale = scaleFromMat4(pose);
            const math::Vec3 currentRotation = rotationFromMat4(pose);

            math::Vec3 targetRotation = currentRotation;
            if (i + 1 < solvedPositions.size()) {
              const math::Vec3 worldDir = solvedPositions[i + 1] - solvedPositions[i];
              const math::Vec3 localDir = transformVector(inverseParent, worldDir);
              targetRotation = directionToRotation(localDir);
              targetRotation.z = currentRotation.z;
            }

            const float weight = std::clamp(chain.weight, 0.0f, 1.0f);
            const math::Vec3 blendedRotation{
                math::lerp(currentRotation, targetRotation, weight).x,
                math::lerp(currentRotation, targetRotation, weight).y,
                math::lerp(currentRotation, targetRotation, weight).z,
            };
            pose = composeLocal(currentTranslation, blendedRotation, currentScale);
            parentWorld = math::mul(parentWorld, pose);
            ik.solvedBoneNames.push_back(skeleton.bones[static_cast<std::size_t>(boneIndex)].key);
          }

          events.emit<ecs::events::IKChainSolvedEvent>({id, chain.name});
        }
      });
}

}  // namespace ecs::systems
