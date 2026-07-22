#include "graphics/OpenGlRenderer.h"

// Author: Karl-Johan Bailey

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "terrain/PerlinNoise2D.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace graphics {

namespace {

struct Vertex final {
  float px, py, pz;
  float nx, ny, nz;
  float u, v;
};

static void glfwErrorCallback(int code, const char* desc) {
  std::cerr << "[glfw] error " << code << ": " << (desc ? desc : "(null)") << "\n";
}

static void getFramebufferSize(GLFWwindow* window, int* outW, int* outH) {
  int w = 1, h = 1;
  glfwGetFramebufferSize(window, &w, &h);
  if (w <= 0) w = 1;
  if (h <= 0) h = 1;
  *outW = w;
  *outH = h;
}

}  // namespace

OpenGlRenderer::~OpenGlRenderer() { stop(); }

bool OpenGlRenderer::start(int width, int height, const char* title) {
  if (m_window) return true;

  glfwSetErrorCallback(glfwErrorCallback);
  if (glfwInit() != GLFW_TRUE) {
    std::cerr << "Failed to initialize GLFW\n";
    return false;
  }

  // macOS supports OpenGL core contexts; request 4.1 core (widely supported on mac).
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(1);
  glfwSetWindowUserPointer(m_window, this);

  getFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
  glViewport(0, 0, m_fbWidth, m_fbHeight);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  return true;
}

void OpenGlRenderer::stop() {
  for (auto& [_, p] : m_programs) {
    destroyProgram(p);
  }
  m_programs.clear();

  for (auto& [_, m] : m_terrainMeshes) {
    destroyTerrainMesh(m);
  }
  m_terrainMeshes.clear();

  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
  glfwTerminate();
}

bool OpenGlRenderer::isOpen() const { return m_window && glfwWindowShouldClose(m_window) == GLFW_FALSE; }

void OpenGlRenderer::pollEvents() {
  if (!m_window) return;
  glfwPollEvents();
  getFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
}

std::string OpenGlRenderer::readTextFile(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return {};
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

std::uint32_t OpenGlRenderer::compileShader(std::uint32_t type, const std::string& source, std::string* outError) {
  const GLuint id = glCreateShader(type);
  const char* src = source.c_str();
  const GLint len = static_cast<GLint>(source.size());
  glShaderSource(id, 1, &src, &len);
  glCompileShader(id);

  GLint ok = 0;
  glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
  if (ok == GL_TRUE) return id;

  GLint logLen = 0;
  glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
  std::string log;
  log.resize(static_cast<std::size_t>(std::max(0, logLen)));
  if (logLen > 0) {
    glGetShaderInfoLog(id, logLen, nullptr, log.data());
  }
  glDeleteShader(id);
  if (outError) *outError = std::move(log);
  return 0;
}

OpenGlRenderer::Program OpenGlRenderer::linkProgram(std::uint32_t vsId, std::uint32_t fsId, std::string* outError) {
  Program p{};
  p.vsId = vsId;
  p.fsId = fsId;
  p.programId = glCreateProgram();
  glAttachShader(p.programId, vsId);
  glAttachShader(p.programId, fsId);
  glLinkProgram(p.programId);

  GLint ok = 0;
  glGetProgramiv(p.programId, GL_LINK_STATUS, &ok);
  if (ok == GL_TRUE) return p;

  GLint logLen = 0;
  glGetProgramiv(p.programId, GL_INFO_LOG_LENGTH, &logLen);
  std::string log;
  log.resize(static_cast<std::size_t>(std::max(0, logLen)));
  if (logLen > 0) {
    glGetProgramInfoLog(p.programId, logLen, nullptr, log.data());
  }

  destroyProgram(p);
  if (outError) *outError = std::move(log);
  return {};
}

void OpenGlRenderer::destroyProgram(Program& p) {
  if (p.programId) glDeleteProgram(p.programId);
  if (p.vsId) glDeleteShader(p.vsId);
  if (p.fsId) glDeleteShader(p.fsId);
  p = {};
}

void OpenGlRenderer::destroyTerrainMesh(TerrainMesh& m) {
  if (m.ebo) glDeleteBuffers(1, &m.ebo);
  if (m.vbo) glDeleteBuffers(1, &m.vbo);
  if (m.vao) glDeleteVertexArrays(1, &m.vao);
  m = {};
}

OpenGlRenderer::Program* OpenGlRenderer::getOrCreateProgram(const std::string& shaderKey) {
  auto it = m_programs.find(shaderKey);
  if (it != m_programs.end()) return &it->second;

  const std::string vsPath = shaderKey + ".vert.glsl";
  const std::string fsPath = shaderKey + ".frag.glsl";
  const std::string vsSrc = readTextFile(vsPath);
  const std::string fsSrc = readTextFile(fsPath);
  if (vsSrc.empty() || fsSrc.empty()) {
    std::cerr << "Shader files missing for key \"" << shaderKey << "\" (expected " << vsPath << ", " << fsPath << ")\n";
    m_programs.emplace(shaderKey, Program{});
    return &m_programs.find(shaderKey)->second;
  }

  std::string err;
  const GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc, &err);
  if (!vs) {
    std::cerr << "Vertex shader compile failed (" << vsPath << "):\n" << err << "\n";
    m_programs.emplace(shaderKey, Program{});
    return &m_programs.find(shaderKey)->second;
  }

  const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc, &err);
  if (!fs) {
    std::cerr << "Fragment shader compile failed (" << fsPath << "):\n" << err << "\n";
    glDeleteShader(vs);
    m_programs.emplace(shaderKey, Program{});
    return &m_programs.find(shaderKey)->second;
  }

  Program p = linkProgram(vs, fs, &err);
  if (!p.programId) {
    std::cerr << "Program link failed (" << shaderKey << "):\n" << err << "\n";
    glDeleteShader(vs);
    glDeleteShader(fs);
    m_programs.emplace(shaderKey, Program{});
    return &m_programs.find(shaderKey)->second;
  }

  auto [insIt, _] = m_programs.emplace(shaderKey, p);
  return &insIt->second;
}

OpenGlRenderer::TerrainMesh* OpenGlRenderer::getOrCreateTerrainMesh(const ecs::systems::GraphicsSystem::TerrainDraw& t) {
  const std::uint32_t id = static_cast<std::uint32_t>(t.entity);
  auto it = m_terrainMeshes.find(id);
  if (it != m_terrainMeshes.end()) {
    auto& m = it->second;
    if (m.gridWidth == t.gridWidth && m.gridHeight == t.gridHeight && m.cellSizeMeters == t.cellSizeMeters &&
        m.heightScaleMeters == t.heightScaleMeters && m.noiseSeed == t.noiseSeed) {
      return &m;
    }
    destroyTerrainMesh(m);
    m_terrainMeshes.erase(it);
  }

  TerrainMesh mesh{};
  mesh.gridWidth = t.gridWidth;
  mesh.gridHeight = t.gridHeight;
  mesh.cellSizeMeters = t.cellSizeMeters;
  mesh.heightScaleMeters = t.heightScaleMeters;
  mesh.noiseSeed = t.noiseSeed;

  const int w = std::max(2, t.gridWidth);
  const int h = std::max(2, t.gridHeight);
  const int vertsW = w + 1;
  const int vertsH = h + 1;

  terrain::PerlinNoise2D noise(mesh.noiseSeed);

  std::vector<float> heights;
  heights.resize(static_cast<std::size_t>(vertsW * vertsH));
  for (int z = 0; z < vertsH; ++z) {
    for (int x = 0; x < vertsW; ++x) {
      const float sx = static_cast<float>(x) * t.cellSizeMeters;
      const float sz = static_cast<float>(z) * t.cellSizeMeters;
      const float n = noise.sampleFractal(sx, sz, t.noise);
      heights[static_cast<std::size_t>(z * vertsW + x)] = n * t.heightScaleMeters;
    }
  }

  std::vector<Vertex> vertices;
  vertices.resize(static_cast<std::size_t>(vertsW * vertsH));

  const float halfW = (static_cast<float>(w) * t.cellSizeMeters) * 0.5f;
  const float halfH = (static_cast<float>(h) * t.cellSizeMeters) * 0.5f;

  auto heightAt = [&](int x, int z) -> float {
    x = std::max(0, std::min(vertsW - 1, x));
    z = std::max(0, std::min(vertsH - 1, z));
    return heights[static_cast<std::size_t>(z * vertsW + x)];
  };

  for (int z = 0; z < vertsH; ++z) {
    for (int x = 0; x < vertsW; ++x) {
      const float px = static_cast<float>(x) * t.cellSizeMeters - halfW;
      const float pz = static_cast<float>(z) * t.cellSizeMeters - halfH;
      const float py = heightAt(x, z);

      // Finite difference normal.
      const float hl = heightAt(x - 1, z);
      const float hr = heightAt(x + 1, z);
      const float hd = heightAt(x, z - 1);
      const float hu = heightAt(x, z + 1);
      const math::Vec3 n = math::normalize(math::Vec3{hl - hr, 2.0f * t.cellSizeMeters, hd - hu});

      Vertex v{};
      v.px = px;
      v.py = py;
      v.pz = pz;
      v.nx = n.x;
      v.ny = n.y;
      v.nz = n.z;
      v.u = static_cast<float>(x) / static_cast<float>(vertsW - 1);
      v.v = static_cast<float>(z) / static_cast<float>(vertsH - 1);
      vertices[static_cast<std::size_t>(z * vertsW + x)] = v;
    }
  }

  std::vector<std::uint32_t> indices;
  indices.reserve(static_cast<std::size_t>(w * h * 6));
  for (int z = 0; z < h; ++z) {
    for (int x = 0; x < w; ++x) {
      const std::uint32_t i0 = static_cast<std::uint32_t>(z * vertsW + x);
      const std::uint32_t i1 = static_cast<std::uint32_t>(z * vertsW + x + 1);
      const std::uint32_t i2 = static_cast<std::uint32_t>((z + 1) * vertsW + x);
      const std::uint32_t i3 = static_cast<std::uint32_t>((z + 1) * vertsW + x + 1);
      indices.push_back(i0);
      indices.push_back(i2);
      indices.push_back(i1);
      indices.push_back(i1);
      indices.push_back(i2);
      indices.push_back(i3);
    }
  }

  mesh.indexCount = static_cast<std::uint32_t>(indices.size());

  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);

  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)), indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 3));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 6));

  glBindVertexArray(0);

  auto [insIt, _] = m_terrainMeshes.emplace(id, mesh);
  return &insIt->second;
}

void OpenGlRenderer::render(const ecs::systems::GraphicsSystem::FrameSnapshot& frame) {
  if (!m_window) return;

  pollEvents();
  glViewport(0, 0, m_fbWidth, m_fbHeight);

  // Basic clear.
  glClearColor(0.55f, 0.72f, 0.92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
  const float fovRad = frame.camera.fovYRadians;
  const math::Mat4 proj = math::perspective(fovRad, aspect, frame.camera.nearClip, frame.camera.farClip);
  const math::Mat4 view =
      math::lookAt(frame.camera.position, frame.camera.position + frame.camera.forward, math::Vec3{0.0f, 1.0f, 0.0f});
  const math::Mat4 viewProj = math::mul(proj, view);

  // Lights: pack (clamp to 16).
  constexpr int kMaxLights = 16;
  const int lightCount = static_cast<int>(std::min<std::size_t>(frame.lights.size(), kMaxLights));
  int types[kMaxLights]{};
  float pos[kMaxLights][3]{};
  float dir[kMaxLights][3]{};
  float col[kMaxLights][3]{};
  float intensity[kMaxLights]{};
  float range[kMaxLights]{};
  for (int i = 0; i < lightCount; ++i) {
    const auto& l = frame.lights[static_cast<std::size_t>(i)];
    types[i] = l.type;
    pos[i][0] = l.position.x;
    pos[i][1] = l.position.y;
    pos[i][2] = l.position.z;
    dir[i][0] = l.direction.x;
    dir[i][1] = l.direction.y;
    dir[i][2] = l.direction.z;
    col[i][0] = l.color.x;
    col[i][1] = l.color.y;
    col[i][2] = l.color.z;
    intensity[i] = l.intensity;
    range[i] = l.range;
  }

  for (const auto& t : frame.terrains) {
    Program* program = getOrCreateProgram(t.shader.key);
    if (!program || !program->programId) continue;

    TerrainMesh* mesh = getOrCreateTerrainMesh(t);
    if (!mesh || !mesh->vao) continue;

    glUseProgram(program->programId);

    // State from ShaderComponent snapshot.
    if (t.doubleSided) {
      glDisable(GL_CULL_FACE);
    } else {
      glEnable(GL_CULL_FACE);
    }

    glDepthMask(t.depthWrite ? GL_TRUE : GL_FALSE);

    const GLint locModel = glGetUniformLocation(program->programId, "u_Model");
    const GLint locViewProj = glGetUniformLocation(program->programId, "u_ViewProj");
    const GLint locCam = glGetUniformLocation(program->programId, "u_CameraPos");
    if (locModel >= 0) {
      const math::Mat4 model = math::translate(t.position);
      glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);
    }
    if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
    if (locCam >= 0) glUniform3f(locCam, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);

    const GLint locBase = glGetUniformLocation(program->programId, "u_BaseColor");
    if (locBase >= 0) glUniform4f(locBase, t.baseColorR, t.baseColorG, t.baseColorB, 1.0f);
    const GLint locRough = glGetUniformLocation(program->programId, "u_Roughness");
    if (locRough >= 0) glUniform1f(locRough, t.roughness);
    const GLint locMet = glGetUniformLocation(program->programId, "u_Metallic");
    if (locMet >= 0) glUniform1f(locMet, t.metallic);

    const GLint locLc = glGetUniformLocation(program->programId, "u_LightCount");
    if (locLc >= 0) glUniform1i(locLc, lightCount);
    const GLint locLt = glGetUniformLocation(program->programId, "u_LightType");
    const GLint locLp = glGetUniformLocation(program->programId, "u_LightPos");
    const GLint locLd = glGetUniformLocation(program->programId, "u_LightDir");
    const GLint locLcol = glGetUniformLocation(program->programId, "u_LightColor");
    const GLint locLi = glGetUniformLocation(program->programId, "u_LightIntensity");
    const GLint locLr = glGetUniformLocation(program->programId, "u_LightRange");
    if (locLt >= 0) glUniform1iv(locLt, lightCount, types);
    if (locLp >= 0) glUniform3fv(locLp, lightCount, &pos[0][0]);
    if (locLd >= 0) glUniform3fv(locLd, lightCount, &dir[0][0]);
    if (locLcol >= 0) glUniform3fv(locLcol, lightCount, &col[0][0]);
    if (locLi >= 0) glUniform1fv(locLi, lightCount, intensity);
    if (locLr >= 0) glUniform1fv(locLr, lightCount, range);

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh->indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }

  glfwSwapBuffers(m_window);

  // Allow escape to close.
  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
  }
}

}  // namespace graphics
