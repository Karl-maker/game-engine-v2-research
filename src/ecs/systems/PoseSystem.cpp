#include "ecs/systems/PoseSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/PoseComponent.h"
#include "ecs/components/SkeletonComponent.h"

#include <algorithm>
#include <cmath>

namespace ecs::systems {

namespace {

math::Vec3 translationFromMat4(const math::Mat4& m) {
  return {m.m[12], m.m[13], m.m[14]};
}

math::Vec3 scaleFromMat4(const math::Mat4& m) {
  const math::Vec3 x{m.m[0], m.m[1], m.m[2]};
  const math::Vec3 y{m.m[4], m.m[5], m.m[6]};
  const math::Vec3 z{m.m[8], m.m[9], m.m[10]};
  return {std::max(0.0001f, math::length(x)), std::max(0.0001f, math::length(y)), std::max(0.0001f, math::length(z))};
}

math::Quat quatFromRotationMatrix(const math::Mat4& m) {
  // m is expected to be a pure rotation matrix (no scale, no translation).
  const float m00 = m.m[0];
  const float m11 = m.m[5];
  const float m22 = m.m[10];
  const float trace = m00 + m11 + m22;
  math::Quat q{};
  if (trace > 0.0f) {
    const float s = std::sqrt(trace + 1.0f) * 2.0f;
    q.w = 0.25f * s;
    q.x = (m.m[9] - m.m[6]) / s;
    q.y = (m.m[2] - m.m[8]) / s;
    q.z = (m.m[4] - m.m[1]) / s;
  } else if (m00 > m11 && m00 > m22) {
    const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
    q.w = (m.m[9] - m.m[6]) / s;
    q.x = 0.25f * s;
    q.y = (m.m[1] + m.m[4]) / s;
    q.z = (m.m[2] + m.m[8]) / s;
  } else if (m11 > m22) {
    const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
    q.w = (m.m[2] - m.m[8]) / s;
    q.x = (m.m[1] + m.m[4]) / s;
    q.y = 0.25f * s;
    q.z = (m.m[6] + m.m[9]) / s;
  } else {
    const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
    q.w = (m.m[4] - m.m[1]) / s;
    q.x = (m.m[2] + m.m[8]) / s;
    q.y = (m.m[6] + m.m[9]) / s;
    q.z = 0.25f * s;
  }
  return math::normalize(q);
}

math::Quat rotationFromMat4(const math::Mat4& m) {
  const math::Vec3 s = scaleFromMat4(m);
  math::Mat4 r = m;
  r.m[0] /= s.x; r.m[1] /= s.x; r.m[2] /= s.x;
  r.m[4] /= s.y; r.m[5] /= s.y; r.m[6] /= s.y;
  r.m[8] /= s.z; r.m[9] /= s.z; r.m[10] /= s.z;
  r.m[12] = r.m[13] = r.m[14] = 0.0f;
  r.m[15] = 1.0f;
  return quatFromRotationMatrix(r);
}

math::Quat quatFromEulerDegrees(const math::Vec3& eulerDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kDegToRad = kPi / 180.0f;
  const float rx = eulerDeg.x * kDegToRad * 0.5f;
  const float ry = eulerDeg.y * kDegToRad * 0.5f;
  const float rz = eulerDeg.z * kDegToRad * 0.5f;
  const float cx = std::cos(rx), sx = std::sin(rx);
  const float cy = std::cos(ry), sy = std::sin(ry);
  const float cz = std::cos(rz), sz = std::sin(rz);

  // Yaw (Y), Pitch (X), Roll (Z) composition: q = qy * qx * qz
  math::Quat q{};
  q.w = cy * cx * cz + sy * sx * sz;
  q.x = cy * sx * cz + sy * cx * sz;
  q.y = sy * cx * cz - cy * sx * sz;
  q.z = cy * cx * sz - sy * sx * cz;
  return math::normalize(q);
}

int findBoneIndex(const ecs::SkeletonComponent& skeleton, const std::string& boneName) {
  int prefixMatch = -1;
  for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
    if (skeleton.bones[i].key == boneName) return static_cast<int>(i);
    if (prefixMatch < 0 && !boneName.empty() && skeleton.bones[i].key.rfind(boneName, 0) == 0) {
      prefixMatch = static_cast<int>(i);
    }
  }
  return prefixMatch;
}

}  // namespace

void PoseSystem::tick(EntityRegistry& registry) const {
  registry.view<ecs::PoseComponent, ecs::SkeletonComponent>(
      [&](ecs::EntityId, ecs::PoseComponent& pose, ecs::SkeletonComponent& skeleton) {
        if (!pose.enabled || !skeleton.enabled) return;
        if (pose.poses.empty()) return;
        if (skeleton.currentPose.size() != skeleton.bones.size()) return;

        for (const auto& p : pose.poses) {
          if (!p.enabled) continue;
          const float poseWeight = std::clamp(p.weight, 0.0f, 1.0f);
          if (poseWeight <= 0.0001f) continue;

          for (const auto& b : p.bones) {
            if (b.boneKey.empty()) continue;
            const int boneIndex = findBoneIndex(skeleton, b.boneKey);
            if (boneIndex < 0 || static_cast<std::size_t>(boneIndex) >= skeleton.currentPose.size()) continue;

            const float w = poseWeight * std::clamp(b.weight, 0.0f, 1.0f);
            if (w <= 0.0001f) continue;

            const math::Mat4 base = skeleton.currentPose[static_cast<std::size_t>(boneIndex)];
            const math::Vec3 baseT = translationFromMat4(base);
            const math::Vec3 baseS = scaleFromMat4(base);
            const math::Quat baseR = rotationFromMat4(base);

            math::Vec3 targetT = baseT;
            math::Vec3 targetS = baseS;
            math::Quat targetR = baseR;

            if (b.hasTranslation) targetT = b.translation;
            if (b.hasScale) targetS = b.scale;
            if (b.hasRotationQuat) targetR = b.rotation;
            else if (b.hasRotationEulerDeg) targetR = quatFromEulerDegrees(b.rotationEulerDeg);

            const math::Vec3 outT = math::lerp(baseT, targetT, w);
            const math::Vec3 outS = math::lerp(baseS, targetS, w);
            const math::Quat outR = math::slerp(baseR, targetR, w);
            skeleton.currentPose[static_cast<std::size_t>(boneIndex)] = math::compose(outT, outR, outS);
          }
        }
      });
}

}  // namespace ecs::systems

