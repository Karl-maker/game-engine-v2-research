#pragma once

// Author: Karl-Johan Bailey
//
// PoseComponent (data only)
// Describes weighted pose overrides applied to a SkeletonComponent by PoseSystem.

#include "math/Mat4.h"
#include "math/Vec3.h"

#include <string>
#include <vector>

namespace ecs {

struct PoseComponent final {
  struct BoneOverride final {
    std::string boneKey;
    float weight = 1.0f;  // 0..1, multiplied by pose weight

    bool hasTranslation = false;
    math::Vec3 translation{0.0f, 0.0f, 0.0f};

    // Authoring-friendly rotation input.
    bool hasRotationEulerDeg = false;
    math::Vec3 rotationEulerDeg{0.0f, 0.0f, 0.0f};

    // For cases where you already have a quaternion.
    bool hasRotationQuat = false;
    math::Quat rotation{};

    bool hasScale = false;
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
  };

  struct Pose final {
    std::string name;
    bool enabled = true;
    float weight = 0.0f;  // 0..1
    std::vector<BoneOverride> bones;
  };

  bool enabled = true;
  std::vector<Pose> poses;
};

}  // namespace ecs

