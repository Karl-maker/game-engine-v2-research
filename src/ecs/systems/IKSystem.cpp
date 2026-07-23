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

static void setTranslation(math::Mat4& m, const math::Vec3& t) {
  m.m[12] = t.x;
  m.m[13] = t.y;
  m.m[14] = t.z;
}

static math::Vec3 transformPoint(const math::Mat4& m, const math::Vec3& p) {
  return {
      m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
      m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
      m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
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
  for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
    if (skeleton.bones[i].key == boneName) return static_cast<int>(i);
  }
  return -1;
}

static math::Mat4 composeWorld(const ecs::TransformComponent& tr) {
  return math::mul(math::translate(tr.position),
                   math::mul(math::rotateY(tr.rotation.y * 3.14159265358979323846f / 180.0f),
                             math::mul(math::rotateX(tr.rotation.x * 3.14159265358979323846f / 180.0f),
                                       math::mul(math::rotateZ(tr.rotation.z * 3.14159265358979323846f / 180.0f),
                                                 math::scale(tr.scale)))));
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
  for (const auto& boneName : boneNames) {
    const int idx = findBoneIndex(skeleton, boneName);
    if (idx >= 0) out.push_back(idx);
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
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
          if (!chain.enabled || chain.boneNames.empty()) continue;

          ecs::EntityId targetEntity = chain.targetEntity;
          if (targetEntity == ecs::kInvalidEntityId && !chain.targetEntityName.empty()) {
            targetEntity = resolveEntityByName(registry, chain.targetEntityName);
          }

          math::Vec3 target = tr.position + chain.targetOffset;
          if (registry.isAlive(targetEntity)) {
            if (const auto* targetTr = registry.tryGet<ecs::TransformComponent>(targetEntity)) {
              target = targetTr->position + chain.targetOffset;
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
            const math::Vec3 solvedLocal = transformPoint(inverseParent, solvedPositions[i]);
            const math::Vec3 currentLocal = translationFromMat4(pose);
            const math::Vec3 localTarget = math::lerp(currentLocal, solvedLocal, std::clamp(chain.weight, 0.0f, 1.0f));
            setTranslation(pose, localTarget);
            parentWorld = math::mul(parentWorld, pose);
            ik.solvedBoneNames.push_back(skeleton.bones[static_cast<std::size_t>(boneIndex)].key);
          }

          events.emit<ecs::events::IKChainSolvedEvent>({id, chain.name});
        }
      });
}

}  // namespace ecs::systems
