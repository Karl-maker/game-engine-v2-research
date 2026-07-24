#pragma once

// Author: Karl-Johan Bailey
//
// TerrainFactory
// Creates a terrain renderable + optional terrain collider from data.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "math/Vec3.h"
#include "terrain/NoiseConfig.h"

#include <string>

namespace ecs::services {

struct TerrainConfig final {
  std::string name;
  math::Vec3 position{0.0f, 0.0f, 0.0f};

  int gridWidth = 96;
  int gridHeight = 96;
  float cellSizeMeters = 1.0f;
  float heightScaleMeters = 2.6f;
  terrain::NoiseConfig noise{.type = terrain::NoiseType::Perlin, .seed = 12345u, .frequency = 0.030f, .octaves = 2, .lacunarity = 2.0f, .persistence = 0.45f};

  float lodMaxRenderDistance = 240.0f;
  float lodStep1Distance = 24.0f;
  float lodStep2Distance = 48.0f;
  float lodStep4Distance = 84.0f;
  float lodStep8Distance = 132.0f;
  float lodStep16Distance = 180.0f;
  float lodForceNearDistance = 18.0f;
  float tessLockDistance = 16.0f;
  float tessEnableDistance = 72.0f;
  float tessDisableDistance = 112.0f;
  float viewDotBias = 0.05f;

  bool colliderEnabled = true;
  float colliderThicknessMeters = 5.0f;

  // Rendering
  std::string shaderKey = "graphics/shaders/terrain";
  bool depthWrite = true;
};

class TerrainFactory final {
 public:
  EntityId create(EntityRegistry& registry, const TerrainConfig& config);
};

}  // namespace ecs::services
