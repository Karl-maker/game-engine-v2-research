#pragma once

// Author: Karl-Johan Bailey

#include "assets/MeshAssetService.h"
#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class SkeletonAssetSyncSystem final {
 public:
  void tick(EntityRegistry& registry, assets::MeshAssetService& assetService) const;
};

}  // namespace ecs::systems
