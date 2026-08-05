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
#include "ecs/components/BillboardComponent.h"
#include "ecs/components/Draw2DComponent.h"
#include "ecs/components/VfxComponent.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Mat4.h"
#include "render/AssetRef.h"
#include "render/Color.h"
#include "terrain/NoiseConfig.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace ecs::systems {

class GraphicsSystem final {
 public:
  struct ActiveCamera final {
    EntityId entity = kInvalidEntityId;
    math::Vec3 position{};
    math::Vec3 forward{0.0f, 0.0f, 1.0f};
    int projectionType = 0;  // ecs::CameraComponent::ProjectionType
    float fovYRadians = 1.0471976f;  // 60 deg
    float orthographicSize = 8.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
    bool useFramebufferAspectRatio = true;

    float renderScale = 1.0f;

    bool depthOfFieldEnabled = false;
    float dofFocusDistance = 6.0f;
    float dofFocusRange = 2.5f;
    float dofBlurStrength = 0.65f;

    bool motionBlurEnabled = false;
    float motionBlurStrength = 0.75f;
    float motionBlurMaxBlurPixels = 18.0f;
    int motionBlurSamples = 12;
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
    bool farLodEnabled = false;
    float farLodStartDistance = 280.0f;
    float farLodEndDistance = 720.0f;
    float farLodBillboardScale = 1.15f;
    float farLodHeightOffset = 0.0f;
    bool farLodCameraFacing = false;
    render::AssetRef farLodTexture{};
    render::Color farLodTint{0.78f, 0.82f, 0.74f, 0.92f};
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
    bool roughnessInvert = false;
    float metallic = 0.0f;
    float specularIntensity = 1.0f;
    float dirtColorNoiseStrength = 0.35f;

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
    bool displacementInvert = false;

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

    // --- Terrain "tile maps" (optional, per-terrain-UV) ---
    // These are intended to be large, non-tiling maps (e.g. 4k/8k) aligned to the terrain UVs.
    // Typical use:
    // - height_map: CPU mesh height sampling + optional shader use
    // - terrain_normal_map: large-scale normal detail
    // - terrain_roughness_map: large-scale roughness variation
    // - terrain_surface_map: grayscale detail (micro variation)
    // - splat_map: RGBA mask for material blending (channel mapping is shader-defined)
    render::AssetRef heightMapTex{};
    bool hasHeightMapTex = false;
    render::AssetRef terrainNormalMapTex{};
    bool hasTerrainNormalMapTex = false;
    render::AssetRef terrainRoughnessMapTex{};
    bool hasTerrainRoughnessMapTex = false;
    render::AssetRef terrainSurfaceMapTex{};
    bool hasTerrainSurfaceMapTex = false;
    render::AssetRef splatMapTex{};
    bool hasSplatMapTex = false;
    render::AssetRef foamNormalTex{};
    bool hasFoamNormalTex = false;
    render::AssetRef rippleMaskTex{};
    bool hasRippleMaskTex = false;

    // Tile-map UV controls.
    float mapUvTilingX = 1.0f;
    float mapUvTilingY = 1.0f;

    // Height map controls (CPU mesh + optional shader usage).
    float heightMapStrength = 1.0f;
    bool heightMapInvert = false;
    float heightMapMipBias = 0.0f;
    int heightMapQuality = 2;  // 0=Low,1=Medium,2=High

    // Terrain normal/roughness/surface controls.
    float terrainNormalMapStrength = 1.0f;
    float terrainRoughnessMapStrength = 1.0f;
    bool terrainRoughnessInvert = false;
    float terrainSurfaceStrength = 0.0f;

    // Water controls (used by the simple ocean shader).
    float shallowColorR = 0.18f;
    float shallowColorG = 0.46f;
    float shallowColorB = 0.52f;
    float foamColorR = 0.92f;
    float foamColorG = 0.96f;
    float foamColorB = 0.98f;
    float waterAlpha = 0.72f;
    float clarity = 0.72f;
    float shoreFadeDistance = 6.0f;
    float shoreFoamDepth = 1.15f;
    float shoreFoamStrength = 0.22f;
    float shoreTerrainBaseY = 0.0f;
    float shoreTerrainHeightScale = 0.0f;
    float waveHeight = 0.0f;
    float waveScale = 0.085f;
    float waveSpeed = 0.28f;
    float waveDirectionX = 1.0f;
    float waveDirectionY = 0.2f;
    float secondaryWaveHeight = 0.0f;
    float secondaryWaveScale = 0.16f;
    float secondaryWaveSpeed = 0.18f;
    float secondaryWaveDirectionX = -0.35f;
    float secondaryWaveDirectionY = 1.0f;
    float rippleTiling = 0.12f;
    float rippleStrength = 0.10f;
    float foamTiling = 0.085f;
    float foamStrength = 0.12f;

    // Splat map controls (channel selection + strength).
    float splatStrength = 1.0f;  // 0=procedural, 1=splat override
    int splatChannel = 0;        // 0=R,1=G,2=B,3=A
    float mapMipBias = 0.0f;     // shader-only mip bias for tile maps
    float mapMipScale = 1.0f;    // shader-only mip scale vs terrain lod step
    float mapMipMax = 8.0f;      // shader-only mip clamp
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
    math::Mat4 modelMatrix = math::identity();
    render::AssetRef meshData{};
    render::AssetRef shader{};
    bool visible = true;
    bool castShadows = true;
    bool receiveShadows = true;
    bool hasBaseColorParam = false;
    float baseColorR = 1.0f;
    float baseColorG = 1.0f;
    float baseColorB = 1.0f;
    bool hasRoughnessParam = false;
    float roughness = 0.85f;
    bool hasMetallicParam = false;
    float metallic = 0.0f;
    bool hasSpecularIntensityParam = false;
    float specularIntensity = 0.08f;
    bool hasNormalStrengthParam = false;
    float normalStrength = 0.22f;
    bool hasAoStrengthParam = false;
    float aoStrength = 1.0f;
    bool hasEmissiveColorParam = false;
    float emissiveColorR = 0.0f;
    float emissiveColorG = 0.0f;
    float emissiveColorB = 0.0f;
    bool hasEmissiveStrengthParam = false;
    float emissiveStrength = 0.0f;
    bool hasDisplacementStrengthParam = false;
    float displacementStrength = 0.0f;
    render::AssetRef albedoTex{};
    render::AssetRef normalTex{};
    render::AssetRef roughnessTex{};
    render::AssetRef metallicTex{};
    render::AssetRef aoTex{};
    render::AssetRef specularTex{};
    render::AssetRef emissiveTex{};
    render::AssetRef displacementTex{};
    render::AssetRef metallicRoughnessTex{};
    render::AssetRef ormTex{};
    bool hasAlbedoTex = false;
    bool hasNormalTex = false;
    bool hasRoughnessTex = false;
    bool hasMetallicTex = false;
    bool hasAoTex = false;
    bool hasSpecularTex = false;
    bool hasEmissiveTex = false;
    bool hasDisplacementTex = false;
    bool hasMetallicRoughnessTex = false;
    bool hasOrmTex = false;
    bool hasSkinning = false;
    std::size_t skinMatrixCount = 0;
    std::array<math::Mat4, 96> skinMatrices{};

    // Optional tessellation controls for shaders that support tessellation.
    float tessNear = 6.0f;
    float tessFar = 120.0f;
    float tessMin = 2.0f;
    float tessMax = 18.0f;
    int tessQuality = 0;
  };

  struct FrameSnapshot final {
    struct RayDraw final {
      EntityId entity = kInvalidEntityId;
      EntityId sensorEntity = kInvalidEntityId;
      math::Vec3 start{};
      math::Vec3 end{};
      render::Color color{0.0f, 1.0f, 0.0f, 1.0f};
      bool hit = false;
      std::string category;
    };

    struct DebugLine final {
      math::Vec3 start{};
      math::Vec3 end{};
      render::Color color{1.0f, 1.0f, 1.0f, 1.0f};
    };

    struct FogDraw final {
      EntityId entity = kInvalidEntityId;
      math::Vec3 anchor{};
      render::Color color{0.70f, 0.78f, 0.92f, 1.0f};
      float density = 0.028f;
      float startDistance = 6.0f;
      float endDistance = 110.0f;
      float maxOpacity = 0.92f;
      float distanceExponent = 1.35f;
      float heightFalloff = 0.085f;
      float baseHeightOffset = -4.0f;
      float horizonStrength = 0.26f;
      float noiseScale = 0.028f;
      float noiseStrength = 0.42f;
      float detailNoiseScale = 0.095f;
      float detailNoiseStrength = 0.18f;
      math::Vec2 windDirection{1.0f, 0.35f};
      float windSpeed = 0.75f;
    };

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
      int skyType = 0;   // ecs::SkyComponent::SkyType
      int cloudType = 0; // ecs::SkyComponent::CloudType
      int quality = 1;   // ecs::SkyComponent::Quality
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

    struct GrassLayerDraw final {
      std::string species;
      float density = 0.0f;
      float minScale = 1.0f;
      float maxScale = 1.0f;
      float bladeSpacing = 1.0f;
      float bendStrength = 0.35f;
      float curveStrength = 0.18f;
      float twistStrength = 0.08f;
      float minSlopeDeg = 0.0f;
      float maxSlopeDeg = 90.0f;
      float minAltitude = -10000.0f;
      float maxAltitude = 10000.0f;
      float noiseScale = 0.06f;
      float noiseStrength = 0.65f;
      float windStrength = 1.0f;
      float maxDistance = 30.0f;
    };

    struct GrassDraw final {
      EntityId entity = kInvalidEntityId;
      EntityId sourceTerrainEntity = kInvalidEntityId;
      math::Vec3 position{};
      math::Vec3 area{18.0f, 0.0f, 18.0f};
      float densityMultiplier = 1.0f;
      std::uint32_t seed = 0u;
      terrain::NoiseConfig densityNoise{};
      float densityNoiseThreshold = 0.42f;
      float densityNoiseContrast = 3.0f;
      float densityNoiseStrength = 1.0f;
      terrain::NoiseConfig islandNoise{};
      math::Vec3 islandNoiseOffset{};
      float islandNoiseThreshold = 0.44f;
      float islandNoiseSoftness = 0.18f;
      float islandNoiseContrast = 1.2f;
      float islandNoiseStrength = 1.0f;
      bool castShadows = false;
      bool receiveShadows = true;
      float lodBias = 1.0f;

      bool interactionEnabled = true;
      float interactionRadiusMeters = 1.25f;
      float interactionStrength = 1.0f;

      render::AssetRef shader{};
      render::AssetRef albedoTex{};
      bool hasAlbedoTex = false;
      float albedoUvScale = 0.22f;  // world->uv scale (meters^-1)

      // Optional explicit texture set for grass shaders. Slots are intended to be:
      // `grass_tex0..grass_tex5` (or a single `grass_albedo`/`albedo` as a fallback).
      // When present, renderers may choose between them per-instance for variation.
      std::vector<render::AssetRef> grassTextures;

      // Optional CPU-sampled density mask. Renderers may use the green channel
      // to boost instance density in "green" areas.
      render::AssetRef densityMaskTex{};
      bool hasDensityMaskTex = false;
      float densityMaskStrength = 0.0f;
      float densityMaskTiling = 1.0f;
      bool densityMaskInvert = false;
      float densityMaskScaleStrength = 0.0f;
      float densityMaskScalePower = 1.0f;

      std::vector<GrassLayerDraw> layers;
    };

    struct Draw2DQuadDraw final {
      EntityId entity = kInvalidEntityId;
      std::string name;
      bool enabled = true;
      int layer = 0;

      ecs::Draw2DComponent::Anchor anchor = ecs::Draw2DComponent::Anchor::TopLeft;
      math::Vec2 offsetPx{0.0f, 0.0f};
      math::Vec2 sizePx{64.0f, 64.0f};

      bool textureEnabled = false;
      render::AssetRef texture{};
      math::Vec2 uv0{0.0f, 0.0f};
      math::Vec2 uv1{1.0f, 1.0f};

      render::Color colorTL{1.0f, 1.0f, 1.0f, 1.0f};
      render::Color colorTR{1.0f, 1.0f, 1.0f, 1.0f};
      render::Color colorBR{1.0f, 1.0f, 1.0f, 1.0f};
      render::Color colorBL{1.0f, 1.0f, 1.0f, 1.0f};
    };

    struct BillboardDraw final {
      EntityId entity = kInvalidEntityId;
      bool enabled = true;
      bool visible = true;
      bool textureEnabled = true;
      bool depthWrite = false;
      bool doubleSided = true;
      ecs::BillboardComponent::FaceMode faceMode = ecs::BillboardComponent::FaceMode::CameraPlane;
      math::Vec3 position{};
      math::Vec3 rotation{};
      math::Vec2 sizeMeters{1.25f, 1.25f};
      math::Vec2 pivot{0.5f, 0.0f};
      float maxRenderDistance = 96.0f;
      render::AssetRef texture{};
      render::AnimatedTexture animatedTexture{};
      render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
    };

    struct VfxDraw final {
      EntityId entity = kInvalidEntityId;
      math::Vec3 position{};
      math::Vec3 rotation{};
      math::Vec3 scale{1.0f, 1.0f, 1.0f};
      ecs::VfxComponent::Type type = ecs::VfxComponent::Type::Fire;
      ecs::VfxComponent::Quality quality = ecs::VfxComponent::Quality::High;
      bool enabled = true;
      bool autoQuality = true;
      float maxRenderDistance = 96.0f;
      float lodNearDistance = 16.0f;
      float lodMidDistance = 32.0f;
      float lodFarDistance = 56.0f;
      float lodUltraDistance = 80.0f;
      float lodForceNearDistance = 8.0f;
      float viewDotBias = 0.0f;
      float intensity = 1.0f;
      float spawnRate = 18.0f;
      float burstInterval = 0.0f;
      float lifetimeSeconds = 1.2f;
      float sizeMeters = 0.35f;
      float sizeVariance = 0.35f;
      float speedMetersPerSecond = 1.4f;
      float speedVariance = 0.45f;
      float gravityScale = 0.35f;
      float drag = 0.08f;
      float flickerStrength = 0.35f;
      float flickerSpeed = 8.0f;
      bool looping = true;
      bool castLight = false;
      std::uint32_t seed = 1337u;
      float heightMeters = 1.25f;
      float upwardBias = 0.65f;
      float spreadRadiusMeters = 0.45f;
      float heatHazeStrength = 0.25f;
      float turbulence = 0.45f;
      float swirlStrength = 0.15f;
      float coreSizeMeters = 0.18f;
      float glowStrength = 1.0f;
      float emberRate = 0.35f;
      float smokeAmount = 0.45f;
      float chargeLengthMeters = 3.5f;
      float arcJitter = 0.45f;
      int branchCount = 4;
      int segmentCount = 8;
      float pulseSpeed = 12.0f;
      float arcThickness = 0.14f;
      float arcGlow = 1.0f;
      int sparkCount = 16;
      float sparkSpreadDegrees = 28.0f;
      float sparkTrailLengthMeters = 0.6f;
      float sparkFadeSeconds = 0.25f;
      float sparkBurstJitter = 0.35f;
      float sparkGravityScale = 1.0f;
      render::Color primaryColor{1.0f, 0.55f, 0.10f, 1.0f};
      render::Color secondaryColor{1.0f, 0.95f, 0.35f, 1.0f};
    };

  struct RenderSettingsDraw final {
    bool present = false;
    bool shadowsEnabled = false;
    int shadowQuality = 1;
    float shadowStrength = 1.0f;
    bool shadowUseTessellation = false;
    bool showRays = false;
    bool showCollisionBoxes = false;
    bool showCombatBoxes = false;
    bool showSkeletonBones = false;
  };

    ActiveCamera camera;
    std::vector<TerrainDraw> terrains;
    std::vector<MeshDraw> meshes;
    std::vector<RayDraw> rays;
    std::vector<FogDraw> fogVolumes;
    std::vector<SkyDraw> skies;
    std::vector<RockDraw> rocks;
    std::vector<GrassDraw> grasses;
    std::vector<Draw2DQuadDraw> draw2d;
    std::vector<BillboardDraw> billboards;
    std::vector<VfxDraw> vfx;
    std::vector<LightDraw> lights;
    RenderSettingsDraw settings{};
    std::vector<DebugLine> debugLines;
  };

  const FrameSnapshot& tick(EntityRegistry& registry);

 private:
  struct MeshTransformCacheEntry final {
    math::Vec3 position{};
    math::Vec3 rotation{};
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
    math::Mat4 modelMatrix = math::identity();
    bool valid = false;
  };

  FrameSnapshot m_frame{};
  std::vector<math::Mat4> m_boneWorldScratch{};
  std::vector<std::uint8_t> m_boneWorldComputedScratch{};
  std::unordered_map<EntityId, MeshTransformCacheEntry> m_meshTransformCache{};
};

}  // namespace ecs::systems
