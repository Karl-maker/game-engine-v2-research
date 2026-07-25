#pragma once

// Author: Karl-Johan Bailey
//
// OpenGlRenderer (demo-focused)
// Minimal window + shader + terrain mesh renderer driven by GraphicsSystem snapshots.

#include "ecs/systems/GraphicsSystem.h"
#include "assets/MeshAssetService.h"
#include "core/FrameDebugger.h"
#include "graphics/ShaderService.h"
#include "graphics/TextureService.h"
#include "render/AnimatedTexture.h"
#include "math/Mat4.h"
#include "math/Vec3.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace graphics {

class OpenGlRenderer final {
 public:
  struct WindowConfig final {
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool vsync = false;
    int refreshRateHz = 0;  // 0 = platform default
  };

  struct RealtimeInput final {
    float moveX = 0.0f;   // -1..1 (A/D)
    float moveZ = 0.0f;   // -1..1 (S/W)
    bool sprint = false;  // Shift
    bool crouch = false;  // Ctrl
    bool jump = false;    // Space
    float mouseDx = 0.0f; // pixels since last drain (only when RMB held)
    float mouseDy = 0.0f; // pixels since last drain (only when RMB held)
    bool lookActive = false;
  };

  OpenGlRenderer() = default;
  ~OpenGlRenderer();

  bool start(int width, int height, const char* title);
  bool start(const WindowConfig& cfg, const char* title);
  void stop();

  bool isOpen() const;
  void pollEvents();

  RealtimeInput drainRealtimeInput();

  void render(const ecs::systems::GraphicsSystem::FrameSnapshot& frame,
              bool debugHudEnabled,
              float fpsEstimate,
              float deltaMs,
              float cpuWorkMs,
              const std::string& extraDebugText = {});

  const core::FrameTimingReport& lastRenderDebugReport() const { return m_renderDebugger.report(); }

 private:
  static OpenGlRenderer* selfFrom(GLFWwindow* w);
  static void glfwKeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
  static void glfwCursorPosCallback(GLFWwindow* w, double x, double y);
  int uniformLocation(std::uint32_t programId, const char* name);
  std::uint32_t requestTextureAsset(const render::AssetRef& texture,
                                    const render::AnimatedTexture* animatedTexture,
                                    bool srgb,
                                    double timeSeconds);
  void pollGpuTimerQueries();
  void beginGpuTimerQuery();
  void endGpuTimerQuery();

  struct TerrainMesh final {
    std::uint32_t vao = 0;
    std::uint32_t vbo = 0;
    std::uint32_t ebo = 0;
    std::uint32_t indexCount = 0;
    int gridWidth = 0;
    int gridHeight = 0;
    float cellSizeMeters = 1.0f;
    float heightScaleMeters = 1.0f;
    std::uint32_t noiseSeed = 1337;
    int lodStep = 1;  // 1=full res, 2=half, 4=quarter, ...
  };

  struct TerrainLodState final {
    int lodStep = 1;
    bool wantTess = true;
  };

  struct RockMesh final {
    struct Chunk final {
      math::Vec3 centerLocal{};
      float radius = 0.0f;
      std::uint32_t instanceOffset = 0;
      std::uint32_t instanceCount = 0;
    };

    std::uint32_t vao = 0;
    std::uint32_t vbo = 0;
    std::uint32_t ebo = 0;
    std::uint32_t instanceVbo = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t instanceCapacity = 0;

    std::uint32_t seed = 0;
    math::Vec3 area{};
    float density = 0.0f;
    float minScale = 0.0f;
    float maxScale = 0.0f;
    float clumpiness = 0.0f;
    float patchScale = 0.0f;

    float chunkSizeMeters = 3.0f;
    std::vector<Chunk> chunks;
  };

  struct GrassMesh final {
    struct Chunk final {
      math::Vec3 centerLocal{};
      float radius = 0.0f;
      std::uint32_t instanceOffset = 0;
      std::uint32_t instanceCount = 0;
    };

    std::uint32_t vao = 0;
    std::uint32_t vbo = 0;
    std::uint32_t ebo = 0;
    std::uint32_t instanceVbo = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t instanceCapacity = 0;

    std::uint32_t seed = 0u;
    math::Vec3 area{};
    float density = 0.0f;
    float minScale = 0.0f;
    float maxScale = 0.0f;
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
    std::string species;

    float chunkSizeMeters = 6.0f;
    std::vector<Chunk> chunks;
  };

  struct GpuSubMesh final {
    std::uint32_t vao = 0;
    std::uint32_t vbo = 0;
    std::uint32_t ebo = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t materialIndex = 0;
  };

  struct GpuMeshAsset final {
    bool ready = false;
    assets::LoadedMeshAsset data{};
    std::vector<GpuSubMesh> subMeshes;
  };

  TerrainMesh* getOrCreateTerrainMesh(const ecs::systems::GraphicsSystem::TerrainDraw& terrain, int lodStep);
  RockMesh* getOrCreateRockMesh(const ecs::systems::GraphicsSystem::FrameSnapshot::RockDraw& rocks,
                                const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain);
  GrassMesh* getOrCreateGrassMesh(const ecs::systems::GraphicsSystem::FrameSnapshot::GrassDraw& grass,
                                  std::size_t layerIndex,
                                  const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain);
  GpuMeshAsset* getOrCreateGpuMesh(const std::string& path);

  static void destroyTerrainMesh(TerrainMesh& m);
  static void destroyRockMesh(RockMesh& m);
  static void destroyGrassMesh(GrassMesh& m);
  static void destroyGpuMesh(GpuMeshAsset& m);

  GLFWwindow* m_window = nullptr;
  int m_fbWidth = 1;
  int m_fbHeight = 1;
  std::string m_baseTitle;
  double m_lastTitleUpdateSeconds = 0.0;

  // Debug overlay (top-left text).
  std::uint32_t m_overlayProgram = 0;
  std::uint32_t m_overlayVao = 0;
  std::uint32_t m_overlayVbo = 0;
  std::size_t m_overlayCapacityVerts = 0;

  // HUD quad rendering.
  std::uint32_t m_hudProgram = 0;
  std::uint32_t m_hudVao = 0;
  std::uint32_t m_hudVbo = 0;
  std::size_t m_hudCapacityVerts = 0;

  // World-space debug line rendering.
  std::uint32_t m_debugLineProgram = 0;
  std::uint32_t m_debugLineVao = 0;
  std::uint32_t m_debugLineVbo = 0;
  std::size_t m_debugLineCapacityVerts = 0;

  // World-space VFX point-sprite rendering.
  std::uint32_t m_vfxProgram = 0;
  std::uint32_t m_vfxVao = 0;
  std::uint32_t m_vfxVbo = 0;
  std::size_t m_vfxCapacityVerts = 0;
  std::string m_gpuVendor;
  std::string m_gpuRenderer;
  std::string m_glVersion;

  // Sky fullscreen triangle VAO.
  std::uint32_t m_skyVao = 0;

  // Shadow map (single directional light, optional).
  std::uint32_t m_shadowFbo = 0;
  std::uint32_t m_shadowDepthTex = 0;
  int m_shadowRes = 0;

  // Scene render targets (for post-processing).
  std::uint32_t m_sceneFbo = 0;
  std::uint32_t m_sceneColorTex = 0;
  std::uint32_t m_sceneDepthTex = 0;
  int m_sceneW = 0;
  int m_sceneH = 0;

  // Single intermediate ping-pong target for post passes.
  std::uint32_t m_postFbo = 0;
  std::uint32_t m_postColorTex = 0;
  int m_postW = 0;
  int m_postH = 0;

  bool m_hasPrevViewProj = false;
  math::Mat4 m_prevViewProj{};

  // Input state (GLFW callbacks write, game drains once per tick).
  bool m_keyW = false;
  bool m_keyA = false;
  bool m_keyS = false;
  bool m_keyD = false;
  bool m_keyShift = false;
  bool m_keyCtrl = false;
  bool m_keySpaceQueued = false;
  bool m_keySpaceHeld = false;
  bool m_cursorCaptured = false;
  bool m_hasMousePos = false;
  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
  double m_accumMouseDx = 0.0;
  double m_accumMouseDy = 0.0;

  ShaderService m_shaders;
  std::unordered_map<std::uint64_t, TerrainMesh> m_terrainMeshes;  // key: (entity id, lodStep)
  std::unordered_map<std::uint32_t, RockMesh> m_rockMeshes;        // key: entity id
  std::unordered_map<std::uint64_t, GrassMesh> m_grassMeshes;      // key: (entity id, layerIndex)
  std::unordered_map<std::string, GpuMeshAsset> m_gpuMeshes;       // key: source path
  std::unordered_map<std::string, assets::MeshAssetService::State> m_meshLogState;
  std::unordered_map<std::uint32_t, std::unordered_map<std::string, int>> m_uniformLocationCache;

  TextureService m_textures;
  assets::MeshAssetService m_meshAssets;
  core::FrameDebugger m_renderDebugger;
  bool m_gpuTimerSupported = false;
  bool m_gpuTimerActive = false;
  std::uint32_t m_gpuTimerQueries[2]{0u, 0u};
  bool m_gpuTimerPending[2]{false, false};
  int m_gpuTimerWriteIndex = 0;
  double m_lastGpuFrameMs = -1.0;
  double m_smoothedGpuFrameMs = -1.0;

  // Runtime LOD state (hysteresis) to avoid flickering when hovering at thresholds.
  std::unordered_map<std::uint32_t, TerrainLodState> m_terrainLodState;  // key: terrain entity id
};

}  // namespace graphics
