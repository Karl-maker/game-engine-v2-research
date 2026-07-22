#pragma once

// Author: Karl-Johan Bailey
//
// OpenGlRenderer (demo-focused)
// Minimal window + shader + terrain mesh renderer driven by GraphicsSystem snapshots.

#include "ecs/systems/GraphicsSystem.h"

#include <cstdint>
#include <string>
#include <unordered_map>

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
  struct Program final {
    std::uint32_t programId = 0;
    std::uint32_t vsId = 0;
    std::uint32_t fsId = 0;
  };

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

  Program* getOrCreateProgram(const std::string& shaderKey);
  TerrainMesh* getOrCreateTerrainMesh(const ecs::systems::GraphicsSystem::TerrainDraw& terrain);

  static std::string readTextFile(const std::string& path);
  static std::string readShaderSourceWithIncludes(const std::string& path);
  static std::uint32_t compileShader(std::uint32_t type, const std::string& source, std::string* outError);
  static Program linkProgram(std::uint32_t vsId, std::uint32_t fsId, std::string* outError);
  static void destroyProgram(Program& p);
  static void destroyTerrainMesh(TerrainMesh& m);

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

  std::unordered_map<std::string, Program> m_programs;
  std::unordered_map<std::uint32_t, TerrainMesh> m_terrainMeshes;  // key: entity id
};

}  // namespace graphics
