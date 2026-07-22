#pragma once

// Author: Karl-Johan Bailey
//
// GraphicsSystem (demo-focused)
// Builds a minimal per-frame "render snapshot" from ECS components.
//
// Current scope (intentionally small):
// - Finds an active camera (CameraComponent + TransformComponent)
// - Collects terrain renderables (TerrainComponent + ShaderComponent + TransformComponent)
// - Collects lights (LightComponent + TransformComponent)

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "math/Vec3.h"
#include "render/AssetRef.h"
#include "terrain/NoiseConfig.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ecs::systems {

class GraphicsSystem final {
 public:
  struct ActiveCamera final {
    EntityId entity = kInvalidEntityId;
    math::Vec3 position{};
    math::Vec3 forward{0.0f, 0.0f, 1.0f};
    float fovYRadians = 1.0471976f;  // 60 deg
    float nearClip = 0.1f;
    float farClip = 1000.0f;
  };

  struct TerrainDraw final {
    EntityId entity = kInvalidEntityId;
    math::Vec3 position{};
    int gridWidth = 0;
    int gridHeight = 0;
    float cellSizeMeters = 1.0f;
    float heightScaleMeters = 1.0f;
    terrain::NoiseConfig noise{};
    std::uint32_t noiseSeed = 1337;
    render::AssetRef shader{};
    int renderMode = 0;  // render::RenderMode (as int)
    int cullMode = 0;    // render::CullMode (as int)
    int depthTest = 0;   // render::DepthTest (as int)
    int blendMode = 0;   // render::BlendMode (as int)
    bool depthWrite = true;
    bool doubleSided = false;
    bool receiveShadows = true;
    bool castShadows = true;
    std::size_t textureCount = 0;
    std::size_t parameterCount = 0;

    // Common parameters extracted from ShaderComponent.parameters (if present).
    float baseColorR = 0.65f;
    float baseColorG = 0.55f;
    float baseColorB = 0.45f;
    float roughness = 1.0f;
    float metallic = 0.0f;
    float specularIntensity = 1.0f;
    float dirtColorNoiseStrength = 0.35f;
    bool dirtSinksEnabled = false;
    float dirtSinkStrength = 0.12f;
    float dirtSinkScale = 1.25f;
    float dirtSinkDensity = 0.35f;

    // Pebbles layer parameters.
    bool pebblesEnabled = false;
    float pebbleColorR = 0.45f;
    float pebbleColorG = 0.42f;
    float pebbleColorB = 0.38f;
    float pebbleRoughness = 0.75f;
    float pebbleScale = 0.25f;
    float pebbleDensity = 0.55f;
    float pebbleBlend = 0.65f;
    float pebbleNormalStrength = 0.6f;
    float pebbleHeight = 0.06f;
  };

  struct LightDraw final {
    EntityId entity = kInvalidEntityId;
    int type = 0;  // ecs::LightComponent::Type (stored as int for isolation)
    math::Vec3 position{};
    math::Vec3 direction{};
    math::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 0.0f;
    bool castShadows = false;
  };

  struct FrameSnapshot final {
    ActiveCamera camera;
    std::vector<TerrainDraw> terrains;
    std::vector<LightDraw> lights;
  };

  const FrameSnapshot& tick(EntityRegistry& registry);

 private:
  FrameSnapshot m_frame{};
};

}  // namespace ecs::systems
