#pragma once

// Author: Karl-Johan Bailey
//
// TerrainFactory
// Spawns a terrain renderable plus optional terrain collider from config data.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"
#include "render/AssetRef.h"
#include "render/Color.h"

#include <optional>

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
  float lodStep32Distance = 260.0f;
  float lodForceNearDistance = 18.0f;
  float tessLockDistance = 16.0f;
  float tessEnableDistance = 72.0f;
  float tessDisableDistance = 112.0f;
  float viewDotBias = 0.05f;
  struct FarLodConfig final {
    bool enabled = false;
    float startDistance = 280.0f;
    float endDistance = 720.0f;
    float billboardScale = 1.15f;
    float heightOffset = 0.0f;
    bool cameraFacing = false;
    render::AssetRef texture{};
    render::Color tint{0.78f, 0.82f, 0.74f, 0.92f};
  } farLod;
  bool hasCollider = true;
  float colliderThicknessMeters = 5.0f;
  physics::LayerMask collisionLayer = physics::kLayerWorld;
  bool hasShader = true;
  // Optional base material preset/descriptor for the terrain surface.
  // Example: materials::presets::HighQualityDirtRockGrassLayer().
  std::optional<ecs::ShaderComponent> material;
  // Terrain shader key used when attaching the render component.
  // Defaults to the terrain shader instead of the generic mesh shader.
  std::string shaderKey = "graphics/shaders/terrain";
  ViewableInput viewable{};
};

class TerrainFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const TerrainConfig& config);
};

}  // namespace ecs::services
