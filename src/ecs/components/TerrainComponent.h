#pragma once

// Author: Karl-Johan Bailey
//
// TerrainComponent (descriptive only)
// Describes the terrain grid and how height should be generated.
// A terrain generation/rendering system interprets this component.

#include "terrain/NoiseConfig.h"
#include "render/AssetRef.h"
#include "render/Color.h"

#include <cstdint>

namespace ecs {

struct TerrainComponent {
  int gridWidth = 512;
  int gridHeight = 512;

  float cellSizeMeters = 1.0f;
  float heightScaleMeters = 150.0f;

  // Seed for the underlying noise generator (separate from NoiseConfig for renderer/physics consistency).
  std::uint32_t noiseSeed = 12345u;

  // Render/cull LOD controls.
  float lodMaxRenderDistance = 240.0f;
  float lodStep1Distance = 24.0f;
  float lodStep2Distance = 48.0f;
  float lodStep4Distance = 84.0f;
  float lodStep8Distance = 132.0f;
  float lodStep16Distance = 180.0f;
  float lodStep32Distance = 260.0f;
  float lodForceNearDistance = 18.0f;
  float tessLockDistance = 16.0f;
  float tessEnableDistance = 72.0f;
  float tessDisableDistance = 112.0f;
  float viewDotBias = 0.05f;

  struct FarLod final {
    bool enabled = false;
    float startDistance = 280.0f;
    float endDistance = 720.0f;
    float billboardScale = 1.15f;
    float heightOffset = 0.0f;
    bool cameraFacing = false;
    render::AssetRef texture{};
    render::Color tint{0.78f, 0.82f, 0.74f, 0.92f};
  } farLod;

  terrain::NoiseConfig noise;
};

}  // namespace ecs
