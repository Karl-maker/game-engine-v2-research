#pragma once

// Author: Karl-Johan Bailey
//
// TargetSystem
// Applies TargetComponent descriptors to entity transforms (orientation only).

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class TargetSystem final {
 public:
  // Updates entity rotations so they face their target entity.
  // Call this once per frame after movement/attachment has positioned entities.
  void update(EntityRegistry& registry, double deltaSeconds);
};

}  // namespace ecs::systems

