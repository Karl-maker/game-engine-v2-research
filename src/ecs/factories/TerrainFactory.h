#pragma once

// Author: Karl-Johan Bailey
//
// TerrainFactory
// Spawns a terrain renderable plus optional terrain collider from config data.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct TerrainConfig final {
  TransformInput transform{.name = "terrain"};
  int gridWidth = 512;
  int gridHeight = 512;
  float cellSizeMeters = 1.0f;
  float heightScaleMeters = 150.0f;
  std::uint32_t noiseSeed = 12345u;
  terrain::NoiseConfig noise{};
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
  bool hasCollider = true;
  float colliderThicknessMeters = 5.0f;
  physics::LayerMask collisionLayer = physics::kLayerWorld;
  bool hasShader = true;
  ViewableInput viewable{};
};

class TerrainFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const TerrainConfig& config);
};

}  // namespace ecs::services
