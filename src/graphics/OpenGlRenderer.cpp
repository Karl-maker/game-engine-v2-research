#include "graphics/OpenGlRenderer.h"

// Author: Karl-Johan Bailey

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "terrain/PerlinNoise2D.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

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

struct RockInstance final {
  float px, py, pz;
  float scale;
  float rot;
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

static std::uint32_t compileGlShader(std::uint32_t type, const std::string& source, std::string* outError) {
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
  if (logLen > 0) glGetShaderInfoLog(id, logLen, nullptr, log.data());
  glDeleteShader(id);
  if (outError) *outError = std::move(log);
  return 0;
}

static std::uint32_t linkGlProgram(std::uint32_t vsId, std::uint32_t fsId, std::string* outError) {
  const GLuint program = glCreateProgram();
  glAttachShader(program, vsId);
  glAttachShader(program, fsId);
  glLinkProgram(program);

  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (ok == GL_TRUE) return program;

  GLint logLen = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
  std::string log;
  log.resize(static_cast<std::size_t>(std::max(0, logLen)));
  if (logLen > 0) glGetProgramInfoLog(program, logLen, nullptr, log.data());
  glDeleteProgram(program);
  if (outError) *outError = std::move(log);
  return 0;
}

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

OpenGlRenderer* OpenGlRenderer::selfFrom(GLFWwindow* w) {
  return w ? static_cast<OpenGlRenderer*>(glfwGetWindowUserPointer(w)) : nullptr;
}

void OpenGlRenderer::glfwKeyCallback(GLFWwindow* w, int key, int, int action, int mods) {
  auto* self = selfFrom(w);
  if (!self) return;
  const bool down = (action != GLFW_RELEASE);
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    self->m_cursorCaptured = !self->m_cursorCaptured;
    glfwSetInputMode(w, GLFW_CURSOR, self->m_cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(w, GLFW_RAW_MOUSE_MOTION, self->m_cursorCaptured ? GLFW_TRUE : GLFW_FALSE);
    }
    self->m_hasMousePos = false;
    self->m_accumMouseDx = 0.0;
    self->m_accumMouseDy = 0.0;
  }
  switch (key) {
    case GLFW_KEY_W: self->m_keyW = down; break;
    case GLFW_KEY_A: self->m_keyA = down; break;
    case GLFW_KEY_S: self->m_keyS = down; break;
    case GLFW_KEY_D: self->m_keyD = down; break;
    default: break;
  }
  self->m_keyShift = (mods & GLFW_MOD_SHIFT) != 0;
  self->m_keyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
}

void OpenGlRenderer::glfwCursorPosCallback(GLFWwindow* w, double x, double y) {
  auto* self = selfFrom(w);
  if (!self) return;
  if (!self->m_cursorCaptured) return;
  if (!self->m_hasMousePos) {
    self->m_hasMousePos = true;
    self->m_lastMouseX = x;
    self->m_lastMouseY = y;
    return;
  }
  const double dx = x - self->m_lastMouseX;
  const double dy = y - self->m_lastMouseY;
  self->m_lastMouseX = x;
  self->m_lastMouseY = y;

  self->m_accumMouseDx += dx;
  self->m_accumMouseDy += dy;
}

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
  glfwSetKeyCallback(m_window, &OpenGlRenderer::glfwKeyCallback);
  glfwSetCursorPosCallback(m_window, &OpenGlRenderer::glfwCursorPosCallback);

  // Capture mouse for FPS-style look by default.
  m_cursorCaptured = true;
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  getFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
  glViewport(0, 0, m_fbWidth, m_fbHeight);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  m_textures.start();

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
    const GLuint vs = compileGlShader(GL_VERTEX_SHADER, vsSrc, &err);
    if (!vs) {
      std::cerr << "Overlay vertex shader compile failed:\n" << err << "\n";
      return true;
    }
    const GLuint fs = compileGlShader(GL_FRAGMENT_SHADER, fsSrc, &err);
    if (!fs) {
      std::cerr << "Overlay fragment shader compile failed:\n" << err << "\n";
      glDeleteShader(vs);
      return true;
    }
    const GLuint prog = linkGlProgram(vs, fs, &err);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) {
      std::cerr << "Overlay program link failed:\n" << err << "\n";
      return true;
    }
    m_overlayProgram = prog;

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

  // Sky VAO (core profile requires a VAO even for gl_VertexID fullscreen triangles).
  glGenVertexArrays(1, &m_skyVao);

  return true;
}

void OpenGlRenderer::stop() {
  m_shaders.clear();

  for (auto& [_, m] : m_terrainMeshes) {
    destroyTerrainMesh(m);
  }
  m_terrainMeshes.clear();

  for (auto& [_, m] : m_rockMeshes) {
    destroyRockMesh(m);
  }
  m_rockMeshes.clear();

  if (m_window) {
    glfwMakeContextCurrent(m_window);
    if (m_skyVao) glDeleteVertexArrays(1, &m_skyVao);
    m_textures.destroyAllGlTextures();
    glfwMakeContextCurrent(nullptr);
  }
  m_textures.stop();

  if (m_overlayVbo) glDeleteBuffers(1, &m_overlayVbo);
  if (m_overlayVao) glDeleteVertexArrays(1, &m_overlayVao);
  if (m_overlayProgram) glDeleteProgram(m_overlayProgram);
  m_overlayVbo = 0;
  m_overlayVao = 0;
  m_overlayProgram = 0;
  m_overlayCapacityVerts = 0;
  m_skyVao = 0;

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

OpenGlRenderer::RealtimeInput OpenGlRenderer::drainRealtimeInput() {
  RealtimeInput out{};
  if (!m_window) return out;

  out.moveX = (m_keyD ? 1.0f : 0.0f) + (m_keyA ? -1.0f : 0.0f);
  out.moveZ = (m_keyW ? 1.0f : 0.0f) + (m_keyS ? -1.0f : 0.0f);
  out.sprint = m_keyShift;
  out.crouch = m_keyCtrl;
  out.lookActive = m_cursorCaptured;
  out.mouseDx = static_cast<float>(m_accumMouseDx);
  out.mouseDy = static_cast<float>(m_accumMouseDy);
  m_accumMouseDx = 0.0;
  m_accumMouseDy = 0.0;
  return out;
}

void OpenGlRenderer::destroyTerrainMesh(TerrainMesh& m) {
  if (m.ebo) glDeleteBuffers(1, &m.ebo);
  if (m.vbo) glDeleteBuffers(1, &m.vbo);
  if (m.vao) glDeleteVertexArrays(1, &m.vao);
  m = {};
}

void OpenGlRenderer::destroyRockMesh(RockMesh& m) {
  if (m.instanceVbo) glDeleteBuffers(1, &m.instanceVbo);
  if (m.ebo) glDeleteBuffers(1, &m.ebo);
  if (m.vbo) glDeleteBuffers(1, &m.vbo);
  if (m.vao) glDeleteVertexArrays(1, &m.vao);
  m = {};
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

OpenGlRenderer::RockMesh* OpenGlRenderer::getOrCreateRockMesh(
    const ecs::systems::GraphicsSystem::FrameSnapshot::RockDraw& r,
    const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain) {
  const std::uint32_t id = static_cast<std::uint32_t>(r.entity);
  auto it = m_rockMeshes.find(id);

  auto needsRebuild = [&](const RockMesh& m) {
    return m.seed != r.seed || m.area.x != r.area.x || m.area.z != r.area.z || m.density != r.density ||
           m.minScale != r.minScale || m.maxScale != r.maxScale || m.clumpiness != r.clumpiness ||
           m.patchScale != r.patchScale;
  };

  if (it != m_rockMeshes.end() && !needsRebuild(it->second)) return &it->second;

  if (it != m_rockMeshes.end()) {
    destroyRockMesh(it->second);
    m_rockMeshes.erase(it);
  }

  RockMesh mesh{};
  mesh.seed = r.seed;
  mesh.area = r.area;
  mesh.density = r.density;
  mesh.minScale = r.minScale;
  mesh.maxScale = r.maxScale;
  mesh.clumpiness = r.clumpiness;
  mesh.patchScale = r.patchScale;
  mesh.chunkSizeMeters = 3.0f;

  struct RockVert {
    float px, py, pz;
    float nx, ny, nz;
  };

  // Low-poly "rock" (icosahedron-ish) as a simple sphere-ish mesh.
  static const RockVert verts[] = {
      {-0.525731f, 0.000000f, 0.850651f, 0, 0, 1},   {0.525731f, 0.000000f, 0.850651f, 0, 0, 1},
      {-0.525731f, 0.000000f, -0.850651f, 0, 0, -1}, {0.525731f, 0.000000f, -0.850651f, 0, 0, -1},
      {0.000000f, 0.850651f, 0.525731f, 0, 1, 0},    {0.000000f, 0.850651f, -0.525731f, 0, 1, 0},
      {0.000000f, -0.850651f, 0.525731f, 0, -1, 0},  {0.000000f, -0.850651f, -0.525731f, 0, -1, 0},
      {0.850651f, 0.525731f, 0.000000f, 1, 0, 0},    {-0.850651f, 0.525731f, 0.000000f, -1, 0, 0},
      {0.850651f, -0.525731f, 0.000000f, 1, 0, 0},   {-0.850651f, -0.525731f, 0.000000f, -1, 0, 0},
  };

  static const std::uint32_t idxs[] = {
      0, 4, 1, 0, 9, 4, 9, 5, 4, 4, 5, 8, 4, 8, 1, 8, 10, 1, 8, 3, 10, 5, 3, 8,
      5, 2, 3, 2, 7, 3, 7, 10, 3, 7, 6, 10, 7, 11, 6, 11, 0, 6, 0, 1, 6, 6, 1, 10,
      9, 0, 11, 9, 11, 2, 9, 2, 5, 7, 2, 11,
  };

  mesh.indexCount = static_cast<std::uint32_t>(sizeof(idxs) / sizeof(idxs[0]));

  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);
  glGenBuffers(1, &mesh.instanceVbo);

  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RockVert), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RockVert), reinterpret_cast<void*>(sizeof(float) * 3));

  glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVbo);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(RockInstance), reinterpret_cast<void*>(0));
  glVertexAttribDivisor(2, 1);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RockInstance), reinterpret_cast<void*>(sizeof(float) * 3));
  glVertexAttribDivisor(3, 1);
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(RockInstance), reinterpret_cast<void*>(sizeof(float) * 4));
  glVertexAttribDivisor(4, 1);

  glBindVertexArray(0);

  // Instance generation (clumpy).
  const float areaM2 = std::max(0.0f, r.area.x) * std::max(0.0f, r.area.z);
  const std::uint32_t maxInstances =
      static_cast<std::uint32_t>(std::min(8000.0f, std::max(0.0f, areaM2 * std::max(0.0f, r.density))));

  terrain::PerlinNoise2D patchNoise(r.seed ^ 0xA341316Cu);
  terrain::PerlinNoise2D heightNoise(groundTerrain ? groundTerrain->noiseSeed : r.seed);
  const terrain::NoiseConfig groundCfg = groundTerrain ? groundTerrain->noise : terrain::NoiseConfig{};
  const float groundHeightScale = groundTerrain ? groundTerrain->heightScaleMeters : 0.0f;

  const float terrainSizeX =
      groundTerrain ? (static_cast<float>(std::max(2, groundTerrain->gridWidth)) * groundTerrain->cellSizeMeters) : 0.0f;
  const float terrainSizeZ =
      groundTerrain ? (static_cast<float>(std::max(2, groundTerrain->gridHeight)) * groundTerrain->cellSizeMeters) : 0.0f;
  const float terrainHalfW = terrainSizeX * 0.5f;
  const float terrainHalfD = terrainSizeZ * 0.5f;

  auto rand01 = [&](std::uint32_t n) {
    n ^= n >> 16;
    n *= 0x7feb352dU;
    n ^= n >> 15;
    n *= 0x846ca68bU;
    n ^= n >> 16;
    return (n & 0x00FFFFFFu) / 16777216.0f;
  };

  const float chunkSize = std::max(2.0f, mesh.chunkSizeMeters);
  const int chunkCountX = std::max(1, static_cast<int>(std::ceil(std::max(0.0f, r.area.x) / chunkSize)));
  const int chunkCountZ = std::max(1, static_cast<int>(std::ceil(std::max(0.0f, r.area.z) / chunkSize)));
  const int chunkCount = chunkCountX * chunkCountZ;
  const float halfW = r.area.x * 0.5f;
  const float halfD = r.area.z * 0.5f;

  struct ChunkBuild final {
    math::Vec3 center{};
    float radius = 0.0f;
    std::vector<RockInstance> instances;
  };

  std::vector<ChunkBuild> chunkBuilds;
  chunkBuilds.resize(static_cast<std::size_t>(chunkCount));
  for (int cz = 0; cz < chunkCountZ; ++cz) {
    for (int cx = 0; cx < chunkCountX; ++cx) {
      const int idx = cz * chunkCountX + cx;
      auto& cb = chunkBuilds[static_cast<std::size_t>(idx)];
      const float x0 = -halfW + static_cast<float>(cx) * chunkSize;
      const float z0 = -halfD + static_cast<float>(cz) * chunkSize;
      const float x1 = std::min(x0 + chunkSize, halfW);
      const float z1 = std::min(z0 + chunkSize, halfD);
      cb.center = {r.position.x + (x0 + x1) * 0.5f, r.position.y, r.position.z + (z0 + z1) * 0.5f};
      const float rx = (x1 - x0) * 0.5f;
      const float rz = (z1 - z0) * 0.5f;
      cb.radius = std::sqrt(rx * rx + rz * rz) + 1.2f;
      cb.instances.reserve(static_cast<std::size_t>(maxInstances / std::max(1, chunkCount)));
    }
  }

  terrain::NoiseConfig clumpCfg;
  clumpCfg.frequency = std::max(0.01f, r.patchScale);
  clumpCfg.octaves = 2;
  clumpCfg.persistence = 0.55f;
  clumpCfg.lacunarity = 2.0f;

  std::uint32_t attempts = 0;
  const std::uint32_t maxAttempts = maxInstances * 8u + 512u;
  std::size_t generated = 0;
  while (generated < static_cast<std::size_t>(maxInstances) && attempts < maxAttempts) {
    const std::uint32_t idx = attempts++;
    const float rx = rand01(r.seed + idx * 9781u);
    const float rz = rand01(r.seed + idx * 6271u);
    const float rScale = rand01(r.seed + idx * 3137u);
    const float rRot = rand01(r.seed + idx * 1951u);

    const float x = (rx * 2.0f - 1.0f) * halfW;
    const float z = (rz * 2.0f - 1.0f) * halfD;

    const float worldX = r.position.x + x;
    const float worldZ = r.position.z + z;
    const float cl = (patchNoise.sampleFractal(worldX, worldZ, clumpCfg) + 1.0f) * 0.5f;
    const float threshold = std::clamp(0.62f + (1.0f - r.clumpiness) * 0.2f, 0.55f, 0.82f);
    if (cl < threshold) continue;

    RockInstance inst{};
    inst.px = worldX;
    inst.pz = worldZ;

    float groundY = groundTerrain ? groundTerrain->position.y : r.position.y;
    if (groundHeightScale != 0.0f) {
      float localX = (inst.px - groundTerrain->position.x) + terrainHalfW;
      float localZ = (inst.pz - groundTerrain->position.z) + terrainHalfD;
      localX = std::clamp(localX, 0.0f, terrainSizeX);
      localZ = std::clamp(localZ, 0.0f, terrainSizeZ);
      groundY += heightNoise.sampleFractal(localX, localZ, groundCfg) * groundHeightScale;
    }
    inst.py = groundY - 0.01f;

    const float s = r.minScale + (r.maxScale - r.minScale) * std::pow(rScale, 1.8f);
    inst.scale = s;
    inst.rot = rRot * 6.2831853f;

    const int cx = std::clamp(static_cast<int>((x + halfW) / chunkSize), 0, chunkCountX - 1);
    const int cz = std::clamp(static_cast<int>((z + halfD) / chunkSize), 0, chunkCountZ - 1);
    chunkBuilds[static_cast<std::size_t>(cz * chunkCountX + cx)].instances.push_back(inst);
    generated += 1;
  }

  std::vector<RockInstance> allInstances;
  allInstances.reserve(maxInstances);
  mesh.chunks.clear();
  mesh.chunks.reserve(chunkBuilds.size());

  for (const auto& cb : chunkBuilds) {
    if (cb.instances.empty()) continue;
    RockMesh::Chunk c;
    c.center = cb.center;
    c.radius = cb.radius;
    c.instanceOffset = static_cast<std::uint32_t>(allInstances.size());
    c.instanceCount = static_cast<std::uint32_t>(cb.instances.size());
    allInstances.insert(allInstances.end(), cb.instances.begin(), cb.instances.end());
    mesh.chunks.push_back(c);
  }

  mesh.instanceCapacity = static_cast<std::uint32_t>(allInstances.size());
  glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(allInstances.size() * sizeof(RockInstance)),
               allInstances.empty() ? nullptr : allInstances.data(), GL_DYNAMIC_DRAW);

  auto [insIt, _] = m_rockMeshes.emplace(id, mesh);
  return &insIt->second;
}

void OpenGlRenderer::render(const ecs::systems::GraphicsSystem::FrameSnapshot& frame,
                            bool debugHudEnabled,
                            float fpsEstimate,
                            float deltaMs,
                            float cpuWorkMs) {
  if (!m_window) return;

  const double renderStart = glfwGetTime();

  m_textures.flushUploads(4);
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

  // Sky pass (fullscreen procedural).
  if (!frame.skies.empty() && m_skyVao) {
    const auto& sky = frame.skies[0];
    const ecs::systems::GraphicsSystem::FrameSnapshot::FogDraw* fog =
        frame.fogVolumes.empty() ? nullptr : &frame.fogVolumes[0];
    const ShaderService::Program* program = m_shaders.getOrCreate(sky.shader.key);
    if (program && program->programId) {
      glUseProgram(program->programId);
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
      glDisable(GL_CULL_FACE);
      glDisable(GL_BLEND);

      const float timeSeconds = static_cast<float>(glfwGetTime());

      // Camera basis for ray generation.
      const math::Vec3 up{0.0f, 1.0f, 0.0f};
      math::Vec3 fwd = math::normalize(frame.camera.forward);
      math::Vec3 right = math::normalize(math::cross(fwd, up));
      if (math::lengthSq(right) < 1e-6f) right = {1.0f, 0.0f, 0.0f};
      math::Vec3 camUp = math::normalize(math::cross(right, fwd));
      const float tanHalfFov = std::tan(frame.camera.fovYRadians * 0.5f);

      const GLint locCamPos = glGetUniformLocation(program->programId, "u_CameraPos");
      if (locCamPos >= 0) glUniform3f(locCamPos, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
      const GLint locFwd = glGetUniformLocation(program->programId, "u_CamForward");
      if (locFwd >= 0) glUniform3f(locFwd, fwd.x, fwd.y, fwd.z);
      const GLint locRight = glGetUniformLocation(program->programId, "u_CamRight");
      if (locRight >= 0) glUniform3f(locRight, right.x, right.y, right.z);
      const GLint locUp = glGetUniformLocation(program->programId, "u_CamUp");
      if (locUp >= 0) glUniform3f(locUp, camUp.x, camUp.y, camUp.z);
      const GLint locAspect = glGetUniformLocation(program->programId, "u_Aspect");
      if (locAspect >= 0) glUniform1f(locAspect, aspect);
      const GLint locTan = glGetUniformLocation(program->programId, "u_TanHalfFov");
      if (locTan >= 0) glUniform1f(locTan, tanHalfFov);
      const GLint locTime = glGetUniformLocation(program->programId, "u_Time");
      if (locTime >= 0) glUniform1f(locTime, timeSeconds);

      const GLint locH = glGetUniformLocation(program->programId, "u_HorizonColor");
      if (locH >= 0) glUniform3f(locH, sky.horizonColor.r, sky.horizonColor.g, sky.horizonColor.b);
      const GLint locZ = glGetUniformLocation(program->programId, "u_ZenithColor");
      if (locZ >= 0) glUniform3f(locZ, sky.zenithColor.r, sky.zenithColor.g, sky.zenithColor.b);
      const GLint locSunOn = glGetUniformLocation(program->programId, "u_SunEnabled");
      if (locSunOn >= 0) glUniform1i(locSunOn, sky.sunEnabled ? 1 : 0);
      const GLint locSunDir = glGetUniformLocation(program->programId, "u_SunDir");
      if (locSunDir >= 0) glUniform3f(locSunDir, sky.sunDirection.x, sky.sunDirection.y, sky.sunDirection.z);
      const GLint locSunTint = glGetUniformLocation(program->programId, "u_SunTint");
      if (locSunTint >= 0) glUniform3f(locSunTint, sky.sunTint.r, sky.sunTint.g, sky.sunTint.b);
      const GLint locSunI = glGetUniformLocation(program->programId, "u_SunDiscIntensity");
      if (locSunI >= 0) glUniform1f(locSunI, sky.sunDiscIntensity);
      const GLint locSunS = glGetUniformLocation(program->programId, "u_SunDiscSize");
      if (locSunS >= 0) glUniform1f(locSunS, sky.sunDiscSize);

      const GLint locSkyType = glGetUniformLocation(program->programId, "u_SkyType");
      if (locSkyType >= 0) glUniform1i(locSkyType, sky.skyType);
      const GLint locCloudOn = glGetUniformLocation(program->programId, "u_CloudsEnabled");
      if (locCloudOn >= 0) glUniform1i(locCloudOn, sky.cloudsEnabled ? 1 : 0);
      const GLint locCloudType = glGetUniformLocation(program->programId, "u_CloudType");
      if (locCloudType >= 0) glUniform1i(locCloudType, sky.cloudType);
      const GLint locQual = glGetUniformLocation(program->programId, "u_CloudQuality");
      if (locQual >= 0) glUniform1i(locQual, sky.quality);
      const GLint locCov = glGetUniformLocation(program->programId, "u_CloudCoverage");
      if (locCov >= 0) glUniform1f(locCov, sky.cloudCoverage);
      const GLint locDen = glGetUniformLocation(program->programId, "u_CloudDensity");
      if (locDen >= 0) glUniform1f(locDen, sky.cloudDensity);
      const GLint locSpd = glGetUniformLocation(program->programId, "u_CloudSpeed");
      if (locSpd >= 0) glUniform1f(locSpd, sky.cloudSpeed);
      const GLint locWind = glGetUniformLocation(program->programId, "u_CloudWindDir");
      if (locWind >= 0) glUniform2f(locWind, sky.cloudWindX, sky.cloudWindZ);
      const GLint locTs = glGetUniformLocation(program->programId, "u_CloudTimeScale");
      if (locTs >= 0) glUniform1f(locTs, sky.cloudTimeScale);
      const GLint locTurb = glGetUniformLocation(program->programId, "u_CloudTurbulence");
      if (locTurb >= 0) glUniform1f(locTurb, sky.cloudTurbulence);
      const GLint locScale = glGetUniformLocation(program->programId, "u_CloudScale");
      if (locScale >= 0) glUniform1f(locScale, sky.cloudScale);
      const GLint locAbs = glGetUniformLocation(program->programId, "u_CloudAbsorption");
      if (locAbs >= 0) glUniform1f(locAbs, sky.cloudLightAbsorption);
      const GLint locCH = glGetUniformLocation(program->programId, "u_CloudHeightMeters");
      if (locCH >= 0) glUniform1f(locCH, sky.cloudHeightMeters);

      const GLint locStarsOn = glGetUniformLocation(program->programId, "u_StarsEnabled");
      if (locStarsOn >= 0) glUniform1i(locStarsOn, sky.starsEnabled ? 1 : 0);
      const GLint locStarsI = glGetUniformLocation(program->programId, "u_StarsIntensity");
      if (locStarsI >= 0) glUniform1f(locStarsI, sky.starsIntensity);
      const GLint locStarsD = glGetUniformLocation(program->programId, "u_StarsDensity");
      if (locStarsD >= 0) glUniform1f(locStarsD, sky.starsDensity);
      const GLint locStarsS = glGetUniformLocation(program->programId, "u_StarsSize");
      if (locStarsS >= 0) glUniform1f(locStarsS, sky.starsSize);
      const GLint locTwS = glGetUniformLocation(program->programId, "u_StarsTwinkleStrength");
      if (locTwS >= 0) glUniform1f(locTwS, sky.starsTwinkleStrength);
      const GLint locTwSpd = glGetUniformLocation(program->programId, "u_StarsTwinkleSpeed");
      if (locTwSpd >= 0) glUniform1f(locTwSpd, sky.starsTwinkleSpeed);
      const GLint locSeed = glGetUniformLocation(program->programId, "u_StarsSeed");
      if (locSeed >= 0) glUniform1ui(locSeed, sky.starsSeed);

      const GLint locFogOn = glGetUniformLocation(program->programId, "u_FogEnabled");
      if (locFogOn >= 0) glUniform1i(locFogOn, fog ? 1 : 0);
      if (fog) {
        const math::Vec3 half = fog->sizeMeters * 0.5f;
        const GLint locFogCenter = glGetUniformLocation(program->programId, "u_FogCenter");
        if (locFogCenter >= 0) glUniform3f(locFogCenter, fog->center.x, fog->center.y, fog->center.z);
        const GLint locFogHalf = glGetUniformLocation(program->programId, "u_FogHalfSize");
        if (locFogHalf >= 0) glUniform3f(locFogHalf, half.x, half.y, half.z);
        const GLint locFogCol = glGetUniformLocation(program->programId, "u_FogColor");
        if (locFogCol >= 0) glUniform3f(locFogCol, fog->color.r, fog->color.g, fog->color.b);
        const GLint locFogDen = glGetUniformLocation(program->programId, "u_FogDensity");
        if (locFogDen >= 0) glUniform1f(locFogDen, fog->density);
        const GLint locFogStart = glGetUniformLocation(program->programId, "u_FogStart");
        if (locFogStart >= 0) glUniform1f(locFogStart, fog->startDistance);
        const GLint locFogEnd = glGetUniformLocation(program->programId, "u_FogEnd");
        if (locFogEnd >= 0) glUniform1f(locFogEnd, fog->endDistance);
        const GLint locFogHf = glGetUniformLocation(program->programId, "u_FogHeightFalloff");
        if (locFogHf >= 0) glUniform1f(locFogHf, fog->heightFalloff);
        const GLint locFogBase = glGetUniformLocation(program->programId, "u_FogBaseHeight");
        if (locFogBase >= 0) glUniform1f(locFogBase, fog->baseHeightOffset);
      }

      glBindVertexArray(m_skyVao);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindVertexArray(0);
    }
  }

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
    const ShaderService::Program* program = m_shaders.getOrCreate(t.shader.key);
    if (!program || !program->programId) continue;

    const ecs::systems::GraphicsSystem::FrameSnapshot::FogDraw* fog =
        frame.fogVolumes.empty() ? nullptr : &frame.fogVolumes[0];

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

    // Fog uniforms.
    const GLint locFogOn = glGetUniformLocation(program->programId, "u_FogEnabled");
    if (locFogOn >= 0) glUniform1i(locFogOn, fog ? 1 : 0);
    if (fog) {
      const math::Vec3 half = fog->sizeMeters * 0.5f;
      const GLint locFogCenter = glGetUniformLocation(program->programId, "u_FogCenter");
      if (locFogCenter >= 0) glUniform3f(locFogCenter, fog->center.x, fog->center.y, fog->center.z);
      const GLint locFogHalf = glGetUniformLocation(program->programId, "u_FogHalfSize");
      if (locFogHalf >= 0) glUniform3f(locFogHalf, half.x, half.y, half.z);
      const GLint locFogCol = glGetUniformLocation(program->programId, "u_FogColor");
      if (locFogCol >= 0) glUniform3f(locFogCol, fog->color.r, fog->color.g, fog->color.b);
      const GLint locFogDen = glGetUniformLocation(program->programId, "u_FogDensity");
      if (locFogDen >= 0) glUniform1f(locFogDen, fog->density);
      const GLint locFogStart = glGetUniformLocation(program->programId, "u_FogStart");
      if (locFogStart >= 0) glUniform1f(locFogStart, fog->startDistance);
      const GLint locFogEnd = glGetUniformLocation(program->programId, "u_FogEnd");
      if (locFogEnd >= 0) glUniform1f(locFogEnd, fog->endDistance);
      const GLint locFogHf = glGetUniformLocation(program->programId, "u_FogHeightFalloff");
      if (locFogHf >= 0) glUniform1f(locFogHf, fog->heightFalloff);
      const GLint locFogBase = glGetUniformLocation(program->programId, "u_FogBaseHeight");
      if (locFogBase >= 0) glUniform1f(locFogBase, fog->baseHeightOffset);
    }

    // Far grass tint (disabled for now).
    const GLint locGtOn = glGetUniformLocation(program->programId, "u_GrassTintEnabled");
    if (locGtOn >= 0) glUniform1i(locGtOn, 0);

    const GLint locBase = glGetUniformLocation(program->programId, "u_BaseColor");
    if (locBase >= 0) glUniform4f(locBase, t.baseColorR, t.baseColorG, t.baseColorB, 1.0f);
    const GLint locRough = glGetUniformLocation(program->programId, "u_Roughness");
    if (locRough >= 0) glUniform1f(locRough, t.roughness);
    const GLint locMet = glGetUniformLocation(program->programId, "u_Metallic");
    if (locMet >= 0) glUniform1f(locMet, t.metallic);
    const GLint locSpec = glGetUniformLocation(program->programId, "u_SpecularIntensity");
    if (locSpec >= 0) glUniform1f(locSpec, t.specularIntensity);

    const GLint locUv = glGetUniformLocation(program->programId, "u_UvTiling");
    if (locUv >= 0) glUniform2f(locUv, t.uvTilingX, t.uvTilingY);
    const GLint locNormStrength = glGetUniformLocation(program->programId, "u_NormalStrength");
    if (locNormStrength >= 0) glUniform1f(locNormStrength, t.normalStrength);
    const GLint locAoStrength = glGetUniformLocation(program->programId, "u_AOStrength");
    if (locAoStrength >= 0) glUniform1f(locAoStrength, t.aoStrength);
    const GLint locDispStrength = glGetUniformLocation(program->programId, "u_DisplacementStrength");
    if (locDispStrength >= 0) glUniform1f(locDispStrength, t.displacementStrength);

    // Textures (async loaded).
    const GLuint albedoId = (t.hasAlbedoTex) ? m_textures.requestTexture(t.albedoTex.key, true) : 0;
    const GLuint normalId = (t.hasNormalTex) ? m_textures.requestTexture(t.normalTex.key, false) : 0;
    const GLuint roughId = (t.hasRoughnessTex) ? m_textures.requestTexture(t.roughnessTex.key, false) : 0;
    const GLuint aoId = (t.hasAoTex) ? m_textures.requestTexture(t.aoTex.key, false) : 0;
    const GLuint dispId = (t.hasDisplacementTex) ? m_textures.requestTexture(t.displacementTex.key, false) : 0;

    const bool useAlbedo = albedoId != 0;
    const bool useNormal = normalId != 0;
    const bool useRough = roughId != 0;
    const bool useAo = aoId != 0;
    const bool useDisp = dispId != 0;

    const GLint locUseAlbedo = glGetUniformLocation(program->programId, "u_UseAlbedo");
    if (locUseAlbedo >= 0) glUniform1i(locUseAlbedo, useAlbedo ? 1 : 0);
    const GLint locUseNormal = glGetUniformLocation(program->programId, "u_UseNormal");
    if (locUseNormal >= 0) glUniform1i(locUseNormal, useNormal ? 1 : 0);
    const GLint locUseRough = glGetUniformLocation(program->programId, "u_UseRoughness");
    if (locUseRough >= 0) glUniform1i(locUseRough, useRough ? 1 : 0);
    const GLint locUseAo = glGetUniformLocation(program->programId, "u_UseAO");
    if (locUseAo >= 0) glUniform1i(locUseAo, useAo ? 1 : 0);
    const GLint locUseDisp = glGetUniformLocation(program->programId, "u_UseDisplacement");
    if (locUseDisp >= 0) glUniform1i(locUseDisp, useDisp ? 1 : 0);

    if (useAlbedo) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, albedoId);
      const GLint loc = glGetUniformLocation(program->programId, "u_Albedo");
      if (loc >= 0) glUniform1i(loc, 0);
    }
    if (useNormal) {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, normalId);
      const GLint loc = glGetUniformLocation(program->programId, "u_NormalTex");
      if (loc >= 0) glUniform1i(loc, 1);
    }
    if (useRough) {
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, roughId);
      const GLint loc = glGetUniformLocation(program->programId, "u_RoughnessTex");
      if (loc >= 0) glUniform1i(loc, 2);
    }
    if (useAo) {
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, aoId);
      const GLint loc = glGetUniformLocation(program->programId, "u_AOTex");
      if (loc >= 0) glUniform1i(loc, 3);
    }
    if (useDisp) {
      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_2D, dispId);
      const GLint loc = glGetUniformLocation(program->programId, "u_DisplacementTex");
      if (loc >= 0) glUniform1i(loc, 4);
    }

    // Rock layer (optional)
    const bool rockEnabled = t.rockLayerEnabled;
    const GLuint rockAlbedoId = (rockEnabled && t.hasRockAlbedoTex) ? m_textures.requestTexture(t.rockAlbedoTex.key, true) : 0;
    const GLuint rockNormalId = (rockEnabled && t.hasRockNormalTex) ? m_textures.requestTexture(t.rockNormalTex.key, false) : 0;
    const GLuint rockRoughId = (rockEnabled && t.hasRockRoughnessTex) ? m_textures.requestTexture(t.rockRoughnessTex.key, false) : 0;
    const GLuint rockAoId = (rockEnabled && t.hasRockAoTex) ? m_textures.requestTexture(t.rockAoTex.key, false) : 0;
    const GLuint rockDispId = (rockEnabled && t.hasRockDisplacementTex) ? m_textures.requestTexture(t.rockDisplacementTex.key, false) : 0;

    const GLint locRockOn = glGetUniformLocation(program->programId, "u_RockLayerEnabled");
    if (locRockOn >= 0) glUniform1i(locRockOn, rockEnabled ? 1 : 0);
    if (rockEnabled) {
      const GLint locRockUv = glGetUniformLocation(program->programId, "u_RockUvTiling");
      if (locRockUv >= 0) glUniform2f(locRockUv, t.rockUvTilingX, t.rockUvTilingY);
      const GLint locRockNorm = glGetUniformLocation(program->programId, "u_RockNormalStrength");
      if (locRockNorm >= 0) glUniform1f(locRockNorm, t.rockNormalStrength);
      const GLint locRockDisp = glGetUniformLocation(program->programId, "u_RockDisplacementStrength");
      if (locRockDisp >= 0) glUniform1f(locRockDisp, t.rockDisplacementStrength);
      const GLint locRockBlend = glGetUniformLocation(program->programId, "u_RockBlendStrength");
      if (locRockBlend >= 0) glUniform1f(locRockBlend, t.rockBlendStrength);
      const GLint locRockNoise = glGetUniformLocation(program->programId, "u_RockNoiseScale");
      if (locRockNoise >= 0) glUniform1f(locRockNoise, t.rockNoiseScale);

      const bool useRockAlbedo = rockAlbedoId != 0;
      const bool useRockNormal = rockNormalId != 0;
      const bool useRockRough = rockRoughId != 0;
      const bool useRockAo = rockAoId != 0;
      const bool useRockDisp = rockDispId != 0;

      const GLint l0 = glGetUniformLocation(program->programId, "u_UseRockAlbedo");
      if (l0 >= 0) glUniform1i(l0, useRockAlbedo ? 1 : 0);
      const GLint l1 = glGetUniformLocation(program->programId, "u_UseRockNormal");
      if (l1 >= 0) glUniform1i(l1, useRockNormal ? 1 : 0);
      const GLint l2 = glGetUniformLocation(program->programId, "u_UseRockRoughness");
      if (l2 >= 0) glUniform1i(l2, useRockRough ? 1 : 0);
      const GLint l3 = glGetUniformLocation(program->programId, "u_UseRockAO");
      if (l3 >= 0) glUniform1i(l3, useRockAo ? 1 : 0);
      const GLint l4 = glGetUniformLocation(program->programId, "u_UseRockDisplacement");
      if (l4 >= 0) glUniform1i(l4, useRockDisp ? 1 : 0);

      if (useRockAlbedo) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, rockAlbedoId);
        const GLint loc = glGetUniformLocation(program->programId, "u_RockAlbedo");
        if (loc >= 0) glUniform1i(loc, 5);
      }
      if (useRockNormal) {
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, rockNormalId);
        const GLint loc = glGetUniformLocation(program->programId, "u_RockNormalTex");
        if (loc >= 0) glUniform1i(loc, 6);
      }
      if (useRockRough) {
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, rockRoughId);
        const GLint loc = glGetUniformLocation(program->programId, "u_RockRoughnessTex");
        if (loc >= 0) glUniform1i(loc, 7);
      }
      if (useRockAo) {
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, rockAoId);
        const GLint loc = glGetUniformLocation(program->programId, "u_RockAOTex");
        if (loc >= 0) glUniform1i(loc, 8);
      }
      if (useRockDisp) {
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, rockDispId);
        const GLint loc = glGetUniformLocation(program->programId, "u_RockDisplacementTex");
        if (loc >= 0) glUniform1i(loc, 9);
      }
    }
    glActiveTexture(GL_TEXTURE0);

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

    // Terrain tessellation (if the shader key provides TCS+TES).
    if (program->hasTessellation) {
      glPatchParameteri(GL_PATCH_VERTICES, 3);
      const GLint locTessNear = glGetUniformLocation(program->programId, "u_TessNear");
      const GLint locTessFar = glGetUniformLocation(program->programId, "u_TessFar");
      const GLint locTessMin = glGetUniformLocation(program->programId, "u_TessMin");
      const GLint locTessMax = glGetUniformLocation(program->programId, "u_TessMax");
      if (locTessNear >= 0) glUniform1f(locTessNear, 6.0f);
      if (locTessFar >= 0) glUniform1f(locTessFar, 120.0f);
      if (locTessMin >= 0) glUniform1f(locTessMin, 2.0f);
      if (locTessMax >= 0) glUniform1f(locTessMax, 18.0f);

      const GLint locNormDispBoost = glGetUniformLocation(program->programId, "u_NormalDisplacementBoost");
      if (locNormDispBoost >= 0) glUniform1f(locNormDispBoost, 0.85f);

      // Use normal maps as additional displacement (micro-height).
      const GLint locNormDerived = glGetUniformLocation(program->programId, "u_NormalDerivedDisplacementStrength");
      if (locNormDerived >= 0) glUniform1f(locNormDerived, 0.55f * t.displacementStrength);
      const GLint locRockNormDerived = glGetUniformLocation(program->programId, "u_RockNormalDerivedDisplacementStrength");
      if (locRockNormDerived >= 0) glUniform1f(locRockNormDerived, 0.75f * t.rockDisplacementStrength);
    }

    glBindVertexArray(mesh->vao);
    const GLenum mode = program->hasTessellation ? GL_PATCHES : GL_TRIANGLES;
    glDrawElements(mode, static_cast<GLsizei>(mesh->indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }

  // Choose a ground terrain to anchor grass height (first terrain for now).
  const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain = frame.terrains.empty() ? nullptr : &frame.terrains[0];

  // Rocks pass (true 3D instances).
  for (const auto& r : frame.rocks) {
    const ShaderService::Program* program = m_shaders.getOrCreate(r.shader.key);
    if (!program || !program->programId) continue;

    RockMesh* mesh = getOrCreateRockMesh(r, groundTerrain);
    if (!mesh || !mesh->vao || mesh->instanceCapacity == 0 || mesh->chunks.empty()) continue;

    glUseProgram(program->programId);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const GLint locViewProj = glGetUniformLocation(program->programId, "u_ViewProj");
    if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
    const GLint locCam = glGetUniformLocation(program->programId, "u_CameraPos");
    if (locCam >= 0) glUniform3f(locCam, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);

    // Simple single directional light.
    math::Vec3 sunDir{-0.2f, -1.0f, -0.3f};
    math::Vec3 sunCol{1.0f, 1.0f, 1.0f};
    float sunIntensity = 1.0f;
    for (const auto& l : frame.lights) {
      if (l.type == 0) {
        sunDir = math::normalize(l.direction);
        sunCol = l.color;
        sunIntensity = l.intensity;
        break;
      }
    }
    const GLint locSunDir = glGetUniformLocation(program->programId, "u_SunDir");
    if (locSunDir >= 0) glUniform3f(locSunDir, sunDir.x, sunDir.y, sunDir.z);
    const GLint locSunCol = glGetUniformLocation(program->programId, "u_SunColor");
    if (locSunCol >= 0) glUniform3f(locSunCol, sunCol.x, sunCol.y, sunCol.z);
    const GLint locSunI = glGetUniformLocation(program->programId, "u_SunIntensity");
    if (locSunI >= 0) glUniform1f(locSunI, sunIntensity);

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->instanceVbo);

    const float lodBias = std::max(0.25f, r.lodBias);
    const float maxDist = 120.0f / lodBias;
    for (const auto& c : mesh->chunks) {
      const math::Vec3 d = frame.camera.position - c.center;
      const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z) - c.radius;
      if (dist > maxDist) continue;

      const std::size_t baseByte = static_cast<std::size_t>(c.instanceOffset) * sizeof(RockInstance);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(RockInstance), reinterpret_cast<void*>(baseByte + 0));
      glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RockInstance),
                            reinterpret_cast<void*>(baseByte + sizeof(float) * 3));
      glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(RockInstance),
                            reinterpret_cast<void*>(baseByte + sizeof(float) * 4));
      glDrawElementsInstanced(GL_TRIANGLES,
                              static_cast<GLsizei>(mesh->indexCount),
                              GL_UNSIGNED_INT,
                              nullptr,
                              static_cast<GLsizei>(c.instanceCount));
    }

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
