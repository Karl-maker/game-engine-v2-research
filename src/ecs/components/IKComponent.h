#pragma once

// Author: Karl-Johan Bailey
//
// IKComponent
// Describes runtime inverse-kinematics chains by bone name.
// The IK system resolves bone names into skeleton indices and updates the skeleton pose.

#include "ecs/EntityId.h"
#include "math/Vec3.h"

#include <string>
#include <vector>

namespace ecs {

struct IKComponent final {
  struct Chain final {
    bool enabled = true;
    std::string name;
    std::vector<std::string> boneNames;

    EntityId targetEntity = kInvalidEntityId;
    std::string targetEntityName;
    math::Vec3 targetOffset{0.0f, 0.0f, 0.0f};
    math::Vec3 targetLocalOffset{0.0f, 0.0f, 0.0f};

    float weight = 1.0f;
    int iterations = 8;
  };

  bool enabled = true;
  std::vector<Chain> chains;

  // Runtime tracking for diagnostics/debugging.
  std::vector<std::string> solvedBoneNames;
};

}  // namespace ecs
