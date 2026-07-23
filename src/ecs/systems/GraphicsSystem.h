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
#include "render/Color.h"
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

    // Texture channels (optional).
    render::AssetRef albedoTex{};
    render::AssetRef normalTex{};
    render::AssetRef roughnessTex{};
    render::AssetRef aoTex{};
    render::AssetRef displacementTex{};
    bool hasAlbedoTex = false;
    bool hasNormalTex = false;
    bool hasRoughnessTex = false;
    bool hasAoTex = false;
    bool hasDisplacementTex = false;

    // UV tiling (shader parameter).
    float uvTilingX = 1.0f;
    float uvTilingY = 1.0f;
    float normalStrength = 1.0f;
    float aoStrength = 0.6f;

    // Displacement influence (shading/bump only).
    float displacementStrength = 0.25f;

    // Tessellation controls (used when the shader supports tessellation).
    // These map directly to the terrain tessellation shader uniforms.
    float tessNear = 6.0f;   // meters
    float tessFar = 120.0f;  // meters
    float tessMin = 2.0f;    // >= 1
    float tessMax = 18.0f;   // <= 64
    int tessQuality = 0;     // 0=Low,1=Medium,2=High (renderer-defined)

    // Optional rock layer (second material set).
    bool rockLayerEnabled = false;
    render::AssetRef rockAlbedoTex{};
    render::AssetRef rockNormalTex{};
    render::AssetRef rockRoughnessTex{};
    render::AssetRef rockAoTex{};
    render::AssetRef rockDisplacementTex{};
    bool hasRockAlbedoTex = false;
    bool hasRockNormalTex = false;
    bool hasRockRoughnessTex = false;
    bool hasRockAoTex = false;
    bool hasRockDisplacementTex = false;
    float rockUvTilingX = 1.0f;
    float rockUvTilingY = 1.0f;
    float rockNormalStrength = 1.0f;
    float rockDisplacementStrength = 0.8f;
    float rockBlendStrength = 0.65f;
    float rockNoiseScale = 0.06f;
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
    std::uint32_t shadowResolution = 1024;
    float shadowBias = 0.001f;
    float shadowDistance = 50.0f;
  };

  struct MeshDraw final {
    EntityId entity = kInvalidEntityId;
    math::Vec3 position{};
    math::Vec3 rotation{};
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
    render::AssetRef meshData{};
    render::AssetRef shader{};
    bool visible = true;
    bool castShadows = true;
    bool receiveShadows = true;
  };

  struct FrameSnapshot final {
    ActiveCamera camera;
    std::vector<TerrainDraw> terrains;
    std::vector<MeshDraw> meshes;

    struct FogDraw final {
      EntityId entity = kInvalidEntityId;
      math::Vec3 center{};
      math::Vec3 sizeMeters{100.0f, 50.0f, 100.0f};
      render::Color color{0.70f, 0.78f, 0.92f, 1.0f};
      float density = 0.012f;
      float startDistance = 18.0f;
      float endDistance = 220.0f;
      float heightFalloff = 0.06f;
      float baseHeightOffset = 0.0f;
    };
    std::vector<FogDraw> fogVolumes;

    struct SkyDraw final {
      EntityId entity = kInvalidEntityId;
      render::AssetRef shader{};
      render::Color horizonColor{0.65f, 0.75f, 0.95f, 1.0f};
      render::Color zenithColor{0.12f, 0.22f, 0.45f, 1.0f};
      bool sunEnabled = true;
      math::Vec3 sunDirection{0.2f, 0.9f, 0.2f};
      render::Color sunTint{1.0f, 0.95f, 0.85f, 1.0f};
      float sunDiscIntensity = 1.0f;
      float sunDiscSize = 1.0f;
      bool cloudsEnabled = true;
      int skyType = 0;       // ecs::SkyComponent::SkyType
      int cloudType = 0;     // ecs::SkyComponent::CloudType
      int quality = 1;       // ecs::SkyComponent::Quality
      float cloudCoverage = 0.35f;
      float cloudDensity = 0.6f;
      float cloudSpeed = 0.02f;
      float cloudWindX = 1.0f;
      float cloudWindZ = 0.6f;
      float cloudTimeScale = 1.0f;
      float cloudTurbulence = 0.35f;
      float cloudScale = 1.0f;
      float cloudLightAbsorption = 0.4f;
      float cloudHeightMeters = 150.0f;

      bool starsEnabled = true;
      float starsIntensity = 0.75f;
      float starsDensity = 0.55f;
      float starsSize = 0.9f;
      float starsTwinkleStrength = 0.25f;
      float starsTwinkleSpeed = 0.6f;
      std::uint32_t starsSeed = 1337u;
    };
    std::vector<SkyDraw> skies;

    struct RockDraw final {
      EntityId entity = kInvalidEntityId;
      math::Vec3 position{};
      math::Vec3 area{12.0f, 0.0f, 12.0f};
      float density = 1.6f;
      std::uint32_t seed = 424242;
      float minScale = 0.04f;
      float maxScale = 0.12f;
      float clumpiness = 0.8f;
      float patchScale = 0.06f;
      float lodBias = 1.0f;
      bool castShadows = false;
      bool receiveShadows = true;
      render::AssetRef shader{};
    };
    std::vector<RockDraw> rocks;
    std::vector<LightDraw> lights;

    struct RenderSettingsDraw final {
      bool present = false;
      bool shadowsEnabled = false;
      int shadowQuality = 1;
      float shadowStrength = 1.0f;
      bool shadowUseTessellation = false;
    };
    RenderSettingsDraw settings{};
  };

  const FrameSnapshot& tick(EntityRegistry& registry);

 private:
  FrameSnapshot m_frame{};
};

}  // namespace ecs::systems
