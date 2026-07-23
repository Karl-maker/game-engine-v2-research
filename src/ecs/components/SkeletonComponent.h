#pragma once

// Author: Karl-Johan Bailey

#include "math/Mat4.h"

#include <string>
#include <vector>

namespace ecs {

struct SkeletonComponent final {
  enum class UpdateMode {
    Always,
    WhenVisible,
    Manual,
  };

  enum class Space {
    Local,
    World,
  };

  struct Bone final {
    std::string key;
    int parentIndex = -1;
    math::Mat4 localBindTransform{};
  };

  struct AnimationClip final {
    enum class Path {
      Translation,
      Rotation,
      Scale,
    };

    struct Channel final {
      int boneIndex = -1;
      Path path = Path::Translation;
      std::vector<float> times;
      std::vector<math::Vec3> vec3Values;
      std::vector<math::Quat> quatValues;
    };

    std::string name;
    float durationSeconds = 0.0f;
    std::vector<Channel> channels;
  };

  bool enabled = true;
  std::string skeletonId;
  std::string skeletonData;
  int rootBone = -1;
  std::vector<Bone> bones;
  int boneCount = 0;
  std::vector<math::Mat4> currentPose;
  std::vector<math::Mat4> bindPose;
  std::vector<math::Mat4> inverseBindMatrices;
  std::vector<AnimationClip> animationClips;
  UpdateMode updateMode = UpdateMode::WhenVisible;
  Space space = Space::Local;
};

}  // namespace ecs
