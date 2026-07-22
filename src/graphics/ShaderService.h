#pragma once

// Author: Karl-Johan Bailey
//
// ShaderService (OpenGL)
// Loads shader source from disk, expands `#include "..."`, and builds GL programs.
// Supports optional tessellation stages when `.tesc.glsl` and `.tese.glsl` exist.

#include <cstdint>
#include <string>
#include <unordered_map>

namespace graphics {

class ShaderService final {
 public:
  struct Program final {
    std::uint32_t programId = 0;
    std::uint32_t vsId = 0;
    std::uint32_t fsId = 0;
    std::uint32_t tcsId = 0;
    std::uint32_t tesId = 0;
    bool hasTessellation = false;
  };

  ShaderService() = default;
  ~ShaderService();

  ShaderService(const ShaderService&) = delete;
  ShaderService& operator=(const ShaderService&) = delete;

  const Program* getOrCreate(const std::string& shaderKey);
  void clear();

 private:
  static std::string readTextFile(const std::string& path);
  static std::string readShaderSourceWithIncludes(const std::string& path);
  static bool fileExists(const std::string& path);
  static std::uint32_t compileShader(std::uint32_t type, const std::string& source, std::string* outError);
  static Program linkProgram(std::uint32_t vsId,
                             std::uint32_t fsId,
                             std::uint32_t tcsId,
                             std::uint32_t tesId,
                             std::string* outError);
  static void destroyProgram(Program& p);

  std::unordered_map<std::string, Program> m_programs;
};

}  // namespace graphics

