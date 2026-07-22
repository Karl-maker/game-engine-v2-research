#pragma once

// Author: Karl-Johan Bailey
//
// OpenGlRenderer (demo-focused)
// Minimal window + shader + terrain mesh renderer driven by GraphicsSystem snapshots.

#include "ecs/systems/GraphicsSystem.h"
#include "graphics/ShaderService.h"
#include "graphics/TextureService.h"
#include "math/Vec3.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace graphics {

class OpenGlRenderer final {
 public:
  OpenGlRenderer() = default;
  ~OpenGlRenderer();

  bool start(int width, int height, const char* title);
  void stop();

  bool isOpen() const;
  void pollEvents();

  void render(const ecs::systems::GraphicsSystem::FrameSnapshot& frame,
              bool debugHudEnabled,
              float fpsEstimate,
              float deltaMs,
              float cpuWorkMs);

 private:
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
  };

  struct RockMesh final {
    struct Chunk final {
      math::Vec3 center{};
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

  TerrainMesh* getOrCreateTerrainMesh(const ecs::systems::GraphicsSystem::TerrainDraw& terrain);
  RockMesh* getOrCreateRockMesh(const ecs::systems::GraphicsSystem::FrameSnapshot::RockDraw& rocks,
                                const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain);

  static void destroyTerrainMesh(TerrainMesh& m);
  static void destroyRockMesh(RockMesh& m);

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
  std::string m_gpuVendor;
  std::string m_gpuRenderer;
  std::string m_glVersion;

  ShaderService m_shaders;
  std::unordered_map<std::uint32_t, TerrainMesh> m_terrainMeshes;  // key: entity id
  std::unordered_map<std::uint32_t, RockMesh> m_rockMeshes;        // key: entity id

  TextureService m_textures;
};

}  // namespace graphics
