#pragma once

// Author: Karl-Johan Bailey
//
// PoseSystem
// Applies PoseComponent overrides onto SkeletonComponent.currentPose.

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class PoseSystem final {
 public:
  void tick(EntityRegistry& registry) const;
};

}  // namespace ecs::systems

