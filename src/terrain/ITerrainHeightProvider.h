#pragma once

// Author: Karl-Johan Bailey
//
// ITerrainHeightProvider
// Agreed interface for querying terrain height.
// A terrain system can implement this by reading TerrainComponent + noise config and generating heights.

#include "ecs/EntityId.h"

namespace terrain {

class ITerrainHeightProvider {
 public:
  virtual ~ITerrainHeightProvider() = default;

  // Returns world-space height at (worldX, worldZ) for the given terrain entity.
  virtual float heightAt(ecs::EntityId terrainEntity, float worldX, float worldZ) const = 0;
};

}  // namespace terrain

