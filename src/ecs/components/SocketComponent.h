#pragma once

// Author: Karl-Johan Bailey
//
// SocketComponent
// Lightweight bone/socket attachment descriptor. A socket entity can target a named entity and skeleton,
// then the socket system stores the computed world transform for attachment or targeting.

#include "ecs/EntityId.h"
#include "math/Mat4.h"
#include "math/Vec3.h"

#include <string>

namespace ecs {

struct SocketComponent final {
  bool enabled = true;

  std::string name;

  EntityId targetEntity = kInvalidEntityId;
  std::string targetEntityName;
  std::string targetMeshId;

  std::string skeletonName;
  std::string boneName;

  math::Vec3 positionOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 scaleOffset{0.0f, 0.0f, 0.0f};

  math::Mat4 worldTransform{};
};

}  // namespace ecs
