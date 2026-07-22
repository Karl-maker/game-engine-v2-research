#include "graphics/OpenGlRenderer.h"

// Author: Karl-Johan Bailey

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "terrain/PerlinNoise2D.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_set>
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

struct OverlayVert final {
  float x;
  float y;
  float r;
  float g;
  float b;
  float a;
};

// 5x7 font. 96 glyphs for ASCII 32..127. Each glyph = 7 rows, 5 bits per row.
// Public-domain style table (compact).
static const unsigned char kFont5x7[96][7] = {
    {0, 0, 0, 0, 0, 0, 0},                    // ' '
    {0x04, 0x04, 0x04, 0x04, 0, 0, 0x04},     // '!'
    {0x0A, 0x0A, 0, 0, 0, 0, 0},              // '"'
    {0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0},  // '#'
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04},  // '$'
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},  // '%'
    {0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D},  // '&'
    {0x06, 0x04, 0x08, 0, 0, 0, 0},              // '''
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},  // '('
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},  // ')'
    {0, 0x04, 0x15, 0x0E, 0x15, 0x04, 0},        // '*'
    {0, 0x04, 0x04, 0x1F, 0x04, 0x04, 0},        // '+'
    {0, 0, 0, 0, 0x06, 0x04, 0x08},              // ','
    {0, 0, 0, 0x1F, 0, 0, 0},                    // '-'
    {0, 0, 0, 0, 0, 0x0C, 0x0C},                 // '.'
    {0x01, 0x02, 0x04, 0x08, 0x10, 0, 0},        // '/'
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},  // '0'
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},  // '1'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},  // '2'
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},  // '3'
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},  // '4'
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},  // '5'
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},  // '6'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},  // '7'
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},  // '8'
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},  // '9'
    {0, 0x0C, 0x0C, 0, 0x0C, 0x0C, 0},           // ':'
    {0, 0x0C, 0x0C, 0, 0x0C, 0x04, 0x08},        // ';'
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},  // '<'
    {0, 0, 0x1F, 0, 0x1F, 0, 0},                 // '='
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},  // '>'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0, 0x04},     // '?'
    {0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0E},  // '@'
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},  // 'A'
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},  // 'B'
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},  // 'C'
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},  // 'D'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},  // 'E'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},  // 'F'
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},  // 'G'
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},  // 'H'
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},  // 'I'
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},  // 'J'
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},  // 'K'
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},  // 'L'
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},  // 'M'
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},  // 'N'
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},  // 'O'
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},  // 'P'
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},  // 'Q'
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},  // 'R'
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},  // 'S'
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},  // 'T'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},  // 'U'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},  // 'V'
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},  // 'W'
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},  // 'X'
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},  // 'Y'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},  // 'Z'
    {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E},  // '['
    {0x10, 0x08, 0x04, 0x02, 0x01, 0, 0},        // '\'
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E},  // ']'
    {0x04, 0x0A, 0x11, 0, 0, 0, 0},              // '^'
    {0, 0, 0, 0, 0, 0, 0x1F},                    // '_'
    {0x08, 0x04, 0x02, 0, 0, 0, 0},              // '`'
    {0, 0, 0x0E, 0x01, 0x0F, 0x11, 0x0F},        // 'a'
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E},  // 'b'
    {0, 0, 0x0E, 0x11, 0x10, 0x11, 0x0E},        // 'c'
    {0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F},  // 'd'
    {0, 0, 0x0E, 0x11, 0x1F, 0x10, 0x0E},        // 'e'
    {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08},  // 'f'
    {0, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E},     // 'g'
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x11},  // 'h'
    {0x04, 0, 0x0C, 0x04, 0x04, 0x04, 0x0E},     // 'i'
    {0x02, 0, 0x06, 0x02, 0x02, 0x12, 0x0C},     // 'j'
    {0x10, 0x10, 0x11, 0x12, 0x1C, 0x12, 0x11},  // 'k'
    {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},  // 'l'
    {0, 0, 0x1A, 0x15, 0x15, 0x11, 0x11},        // 'm'
    {0, 0, 0x1E, 0x11, 0x11, 0x11, 0x11},        // 'n'
    {0, 0, 0x0E, 0x11, 0x11, 0x11, 0x0E},        // 'o'
    {0, 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10},     // 'p'
    {0, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x01},     // 'q'
    {0, 0, 0x16, 0x19, 0x10, 0x10, 0x10},        // 'r'
    {0, 0, 0x0F, 0x10, 0x0E, 0x01, 0x1E},        // 's'
    {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06},  // 't'
    {0, 0, 0x11, 0x11, 0x11, 0x11, 0x0F},        // 'u'
    {0, 0, 0x11, 0x11, 0x11, 0x0A, 0x04},        // 'v'
    {0, 0, 0x11, 0x11, 0x15, 0x15, 0x0A},        // 'w'
    {0, 0, 0x11, 0x0A, 0x04, 0x0A, 0x11},        // 'x'
    {0, 0x11, 0x11, 0x11, 0x0F, 0x01, 0x0E},     // 'y'
    {0, 0, 0x1F, 0x02, 0x04, 0x08, 0x1F},        // 'z'
    {0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02},  // '{'
    {0x04, 0x04, 0x04, 0, 0x04, 0x04, 0x04},     // '|'
    {0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08},  // '}'
    {0x08, 0x15, 0x02, 0, 0, 0, 0},              // '~'
    {0, 0, 0, 0, 0, 0, 0},                        // DEL
};

static void pushQuad(std::vector<OverlayVert>& verts,
                     float x0,
                     float y0,
                     float x1,
                     float y1,
                     float r,
                     float g,
                     float b,
                     float a) {
  verts.push_back({x0, y0, r, g, b, a});
  verts.push_back({x1, y0, r, g, b, a});
  verts.push_back({x1, y1, r, g, b, a});
  verts.push_back({x0, y0, r, g, b, a});
  verts.push_back({x1, y1, r, g, b, a});
  verts.push_back({x0, y1, r, g, b, a});
}

static void buildTextQuads(std::vector<OverlayVert>& verts,
                           int fbW,
                           int fbH,
                           float xPx,
                           float yPx,
                           float scalePx,
                           const std::string& text,
                           float r,
                           float g,
                           float b,
                           float a) {
  const float glyphW = 6.0f * scalePx;
  const float glyphH = 8.0f * scalePx;

  float penX = xPx;
  float penY = yPx;
  for (char ch : text) {
    if (ch == '\n') {
      penX = xPx;
      penY += glyphH + 2.0f * scalePx;
      continue;
    }
    if (ch < 32 || ch > 127) ch = '?';
    const unsigned char* rows = kFont5x7[static_cast<int>(ch) - 32];

    for (int row = 0; row < 7; ++row) {
      const unsigned char bits = rows[row];
      for (int col = 0; col < 5; ++col) {
        const bool on = (bits & (1u << (4 - col))) != 0;
        if (!on) continue;

        const float x0 = penX + static_cast<float>(col) * scalePx;
        const float y0 = penY + static_cast<float>(row) * scalePx;
        const float x1 = x0 + scalePx;
        const float y1 = y0 + scalePx;

        // Convert to NDC with (0,0) at top-left.
        const float ndcX0 = (x0 / static_cast<float>(fbW)) * 2.0f - 1.0f;
        const float ndcX1 = (x1 / static_cast<float>(fbW)) * 2.0f - 1.0f;
        const float ndcY0 = 1.0f - (y0 / static_cast<float>(fbH)) * 2.0f;
        const float ndcY1 = 1.0f - (y1 / static_cast<float>(fbH)) * 2.0f;
        pushQuad(verts, ndcX0, ndcY1, ndcX1, ndcY0, r, g, b, a);
      }
    }

    penX += glyphW;
  }
}

}  // namespace

OpenGlRenderer::~OpenGlRenderer() { stop(); }

bool OpenGlRenderer::start(int width, int height, const char* title) {
  if (m_window) return true;
  m_baseTitle = title ? title : "Duppy";
  m_lastTitleUpdateSeconds = 0.0;

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

  // Cache GPU strings (available after context creation).
  if (const auto* s = glGetString(GL_VENDOR)) m_gpuVendor = reinterpret_cast<const char*>(s);
  if (const auto* s = glGetString(GL_RENDERER)) m_gpuRenderer = reinterpret_cast<const char*>(s);
  if (const auto* s = glGetString(GL_VERSION)) m_glVersion = reinterpret_cast<const char*>(s);

  // Debug overlay program (simple colored quads in NDC).
  {
    const std::string vsSrc =
        "#version 410 core\n"
        "layout(location=0) in vec2 a_Pos;\n"
        "layout(location=1) in vec4 a_Color;\n"
        "out vec4 v_Color;\n"
        "void main(){ v_Color=a_Color; gl_Position=vec4(a_Pos,0.0,1.0); }\n";
    const std::string fsSrc =
        "#version 410 core\n"
        "in vec4 v_Color;\n"
        "out vec4 o_Color;\n"
        "void main(){ o_Color=v_Color; }\n";

    std::string err;
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc, &err);
    if (!vs) {
      std::cerr << "Overlay vertex shader compile failed:\n" << err << "\n";
      return true;
    }
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc, &err);
    if (!fs) {
      std::cerr << "Overlay fragment shader compile failed:\n" << err << "\n";
      glDeleteShader(vs);
      return true;
    }
    Program p = linkProgram(vs, fs, &err);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!p.programId) {
      std::cerr << "Overlay program link failed:\n" << err << "\n";
      return true;
    }
    m_overlayProgram = p.programId;

    glGenVertexArrays(1, &m_overlayVao);
    glGenBuffers(1, &m_overlayVbo);
    glBindVertexArray(m_overlayVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(OverlayVert), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(OverlayVert),
                          reinterpret_cast<void*>(sizeof(float) * 2));
    glBindVertexArray(0);
    m_overlayCapacityVerts = 0;
  }

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

  if (m_overlayVbo) glDeleteBuffers(1, &m_overlayVbo);
  if (m_overlayVao) glDeleteVertexArrays(1, &m_overlayVao);
  if (m_overlayProgram) glDeleteProgram(m_overlayProgram);
  m_overlayVbo = 0;
  m_overlayVao = 0;
  m_overlayProgram = 0;
  m_overlayCapacityVerts = 0;

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

std::string OpenGlRenderer::readShaderSourceWithIncludes(const std::string& path) {
  std::unordered_set<std::string> seen;

  std::function<std::string(const std::string&)> expand = [&](const std::string& p) -> std::string {
    if (seen.count(p)) {
      std::cerr << "Shader include cycle detected at: " << p << "\n";
      return {};
    }
    seen.insert(p);

    const std::string src = readTextFile(p);
    if (src.empty()) return {};

    // Directory for relative includes.
    std::string dir;
    if (auto slash = p.find_last_of("/\\"); slash != std::string::npos) {
      dir = p.substr(0, slash + 1);
    }

    std::stringstream in(src);
    std::stringstream out;
    std::string line;
    while (std::getline(in, line)) {
      const std::string includePrefix = "#include \"";
      if (line.rfind(includePrefix, 0) == 0) {
        const auto endQuote = line.find("\"", includePrefix.size());
        if (endQuote != std::string::npos) {
          const std::string includePathRaw = line.substr(includePrefix.size(), endQuote - includePrefix.size());
          const bool isAbsoluteLike = (!includePathRaw.empty() && (includePathRaw[0] == '/' || includePathRaw[0] == '\\'));
          const std::string includePath = isAbsoluteLike ? includePathRaw : (dir + includePathRaw);
          out << expand(includePath) << "\n";
          continue;
        }
      }
      out << line << "\n";
    }

    return out.str();
  };

  return expand(path);
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
  const std::string vsSrc = readShaderSourceWithIncludes(vsPath);
  const std::string fsSrc = readShaderSourceWithIncludes(fsPath);
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

void OpenGlRenderer::render(const ecs::systems::GraphicsSystem::FrameSnapshot& frame,
                            bool debugHudEnabled,
                            float fpsEstimate,
                            float deltaMs,
                            float cpuWorkMs) {
  if (!m_window) return;

  const double renderStart = glfwGetTime();

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

    const float timeSeconds = static_cast<float>(glfwGetTime());

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
    const GLint locTime = glGetUniformLocation(program->programId, "u_Time");
    const GLint locColorNoise = glGetUniformLocation(program->programId, "u_DirtColorNoiseStrength");
    const GLint locSinkOn = glGetUniformLocation(program->programId, "u_DirtSinksEnabled");
    const GLint locSinkStrength = glGetUniformLocation(program->programId, "u_DirtSinkStrength");
    const GLint locSinkScale = glGetUniformLocation(program->programId, "u_DirtSinkScale");
    const GLint locSinkDensity = glGetUniformLocation(program->programId, "u_DirtSinkDensity");
    if (locModel >= 0) {
      const math::Mat4 model = math::translate(t.position);
      glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);
    }
    if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
    if (locCam >= 0) glUniform3f(locCam, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
    if (locTime >= 0) glUniform1f(locTime, timeSeconds);
    if (locColorNoise >= 0) glUniform1f(locColorNoise, t.dirtColorNoiseStrength);
    if (locSinkOn >= 0) glUniform1i(locSinkOn, t.dirtSinksEnabled ? 1 : 0);
    if (locSinkStrength >= 0) glUniform1f(locSinkStrength, t.dirtSinkStrength);
    if (locSinkScale >= 0) glUniform1f(locSinkScale, t.dirtSinkScale);
    if (locSinkDensity >= 0) glUniform1f(locSinkDensity, t.dirtSinkDensity);

    const GLint locBase = glGetUniformLocation(program->programId, "u_BaseColor");
    if (locBase >= 0) glUniform4f(locBase, t.baseColorR, t.baseColorG, t.baseColorB, 1.0f);
    const GLint locRough = glGetUniformLocation(program->programId, "u_Roughness");
    if (locRough >= 0) glUniform1f(locRough, t.roughness);
    const GLint locMet = glGetUniformLocation(program->programId, "u_Metallic");
    if (locMet >= 0) glUniform1f(locMet, t.metallic);
    const GLint locSpec = glGetUniformLocation(program->programId, "u_SpecularIntensity");
    if (locSpec >= 0) glUniform1f(locSpec, t.specularIntensity);

    const GLint locUseAlbedo = glGetUniformLocation(program->programId, "u_UseAlbedo");
    if (locUseAlbedo >= 0) glUniform1i(locUseAlbedo, 0);

    // Pebbles layer uniforms (optional).
    const GLint locPebOn = glGetUniformLocation(program->programId, "u_PebblesEnabled");
    if (locPebOn >= 0) glUniform1i(locPebOn, t.pebblesEnabled ? 1 : 0);
    const GLint locPebCol = glGetUniformLocation(program->programId, "u_PebbleColor");
    if (locPebCol >= 0) glUniform3f(locPebCol, t.pebbleColorR, t.pebbleColorG, t.pebbleColorB);
    const GLint locPebR = glGetUniformLocation(program->programId, "u_PebbleRoughness");
    if (locPebR >= 0) glUniform1f(locPebR, t.pebbleRoughness);
    const GLint locPebScale = glGetUniformLocation(program->programId, "u_PebbleScale");
    if (locPebScale >= 0) glUniform1f(locPebScale, t.pebbleScale);
    const GLint locPebDen = glGetUniformLocation(program->programId, "u_PebbleDensity");
    if (locPebDen >= 0) glUniform1f(locPebDen, t.pebbleDensity);
    const GLint locPebBlend = glGetUniformLocation(program->programId, "u_PebbleBlend");
    if (locPebBlend >= 0) glUniform1f(locPebBlend, t.pebbleBlend);
    const GLint locPebNs = glGetUniformLocation(program->programId, "u_PebbleNormalStrength");
    if (locPebNs >= 0) glUniform1f(locPebNs, t.pebbleNormalStrength);
    const GLint locPebH = glGetUniformLocation(program->programId, "u_PebbleHeight");
    if (locPebH >= 0) glUniform1f(locPebH, t.pebbleHeight);

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

  // Allow escape to close.
  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
  }

  const float renderMsSoFar = static_cast<float>((glfwGetTime() - renderStart) * 1000.0);

  if (debugHudEnabled && m_overlayProgram && m_overlayVao && m_overlayVbo) {
    std::string overlay;
    if (!m_gpuRenderer.empty()) {
      overlay += "GPU: " + m_gpuRenderer;
      if (!m_gpuVendor.empty()) overlay += " (" + m_gpuVendor + ")";
      overlay += "\n";
    }
    if (!m_glVersion.empty()) {
      overlay += "GL: " + m_glVersion + "\n";
    }
    overlay += "fps=" + std::to_string(static_cast<int>(fpsEstimate + 0.5f));
    overlay += "  dt_ms=" + std::to_string(static_cast<int>(deltaMs + 0.5f));
    overlay += "  cpu_ms=" + std::to_string(static_cast<int>(cpuWorkMs + 0.5f));
    overlay += "  render_ms=" + std::to_string(static_cast<int>(renderMsSoFar + 0.5f));

    std::vector<OverlayVert> verts;
    verts.reserve(4096);

    const float scalePx = 2.0f;
    const float x0 = 10.0f;
    const float y0 = 10.0f;

    // Shadow first.
    buildTextQuads(verts, m_fbWidth, m_fbHeight, x0 + 1.0f, y0 + 1.0f, scalePx, overlay, 0.0f, 0.0f, 0.0f, 0.75f);
    // Main text.
    buildTextQuads(verts, m_fbWidth, m_fbHeight, x0, y0, scalePx, overlay, 0.95f, 0.95f, 0.95f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_overlayProgram);
    glBindVertexArray(m_overlayVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVbo);

    if (verts.size() > m_overlayCapacityVerts) {
      m_overlayCapacityVerts = std::max<std::size_t>(verts.size(), m_overlayCapacityVerts * 2 + 1024);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_overlayCapacityVerts * sizeof(OverlayVert)), nullptr,
                   GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(OverlayVert)), verts.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
  }

  glfwSwapBuffers(m_window);

  const double renderEnd = glfwGetTime();
  const float renderMs = static_cast<float>((renderEnd - renderStart) * 1000.0);

  const double now = renderEnd;
  const bool shouldUpdateTitle = (now - m_lastTitleUpdateSeconds) >= 0.25;
  if (debugHudEnabled) {
    if (shouldUpdateTitle) {
      m_lastTitleUpdateSeconds = now;
      std::string title = m_baseTitle;
      title += " | fps=" + std::to_string(static_cast<int>(fpsEstimate + 0.5f));
      title += " dt_ms=" + std::to_string(static_cast<int>(deltaMs + 0.5f));
      title += " cpu_ms=" + std::to_string(static_cast<int>(cpuWorkMs + 0.5f));
      title += " render_ms=" + std::to_string(static_cast<int>(renderMs + 0.5f));
      glfwSetWindowTitle(m_window, title.c_str());
    }
  } else {
    if (shouldUpdateTitle) {
      m_lastTitleUpdateSeconds = now;
      glfwSetWindowTitle(m_window, m_baseTitle.c_str());
    }
  }
}

}  // namespace graphics
