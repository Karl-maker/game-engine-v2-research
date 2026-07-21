#pragma once

// Author: Karl-Johan Bailey
//
// TerrainComponent (descriptive only)
// Describes the terrain grid and how height should be generated.
// A terrain generation/rendering system interprets this component.

#include "terrain/NoiseConfig.h"

namespace ecs {

struct TerrainComponent {
  int gridWidth = 512;
  int gridHeight = 512;

  float cellSizeMeters = 1.0f;
  float heightScaleMeters = 150.0f;

  terrain::NoiseConfig noise;
};

}  // namespace ecs

