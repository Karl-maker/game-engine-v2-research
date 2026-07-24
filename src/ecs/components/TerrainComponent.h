#pragma once

// Author: Karl-Johan Bailey
//
// TerrainComponent (descriptive only)
// Describes the terrain grid and how height should be generated.
// A terrain generation/rendering system interprets this component.

#include "terrain/NoiseConfig.h"

#include <cstdint>

namespace ecs {

struct TerrainComponent {
  int gridWidth = 512;
  int gridHeight = 512;

  float cellSizeMeters = 1.0f;
  float heightScaleMeters = 150.0f;

  // Seed for the underlying noise generator (separate from NoiseConfig for renderer/physics consistency).
  std::uint32_t noiseSeed = 12345u;

  terrain::NoiseConfig noise;
};

}  // namespace ecs
