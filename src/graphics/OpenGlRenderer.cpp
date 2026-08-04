#include "graphics/OpenGlRenderer.h"

// Author: Karl-Johan Bailey

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "terrain/PerlinNoise2D.h"

#include <algorithm>
#include <functional>
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

static constexpr const char* kDefaultGrassBillboardTexture =
    "assets/textures/vegitation/grass_patch_02/Material_baseColor.png";

struct GrassBillboardFactory final {
  struct PlaneSpec final {
    float yawDeg = 0.0f;
    math::Vec3 offset{};
    float heightMul = 1.0f;
  };

  static PlaneSpec planeSpec(int planeId, float setupJitter) {
    PlaneSpec spec{};
    if (planeId == 0) {
      spec.yawDeg = -23.0f + setupJitter * 12.0f;
      spec.offset = {-0.045f, 0.0f, 0.020f};
      spec.heightMul = 1.0f;
    } else if (planeId == 1) {
      spec.yawDeg = 31.0f - setupJitter * 16.0f;
      spec.offset = {0.030f, 0.0f, -0.032f};
      spec.heightMul = 0.90f;
    } else {
      spec.yawDeg = 71.0f + setupJitter * 10.0f;
      spec.offset = {-0.012f, 0.0f, 0.048f};
      spec.heightMul = 1.08f;
    }
    return spec;
  }
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

static math::Mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
  math::Mat4 out{};
  for (float& v : out.m) v = 0.0f;

  const float rl = right - left;
  const float tb = top - bottom;
  const float fn = zFar - zNear;
  out.m[0] = 2.0f / std::max(1e-6f, rl);
  out.m[5] = 2.0f / std::max(1e-6f, tb);
  out.m[10] = -2.0f / std::max(1e-6f, fn);
  out.m[12] = -(right + left) / std::max(1e-6f, rl);
  out.m[13] = -(top + bottom) / std::max(1e-6f, tb);
  out.m[14] = -(zFar + zNear) / std::max(1e-6f, fn);
  out.m[15] = 1.0f;
  return out;
}

static math::Mat4 composeTransform(const math::Vec3& position, const math::Vec3& rotationDeg, const math::Vec3& scale) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kDegToRad = kPi / 180.0f;
  return math::mul(math::translate(position),
                   math::mul(math::rotateY(rotationDeg.y * kDegToRad),
                             math::mul(math::rotateX(rotationDeg.x * kDegToRad),
                                       math::mul(math::rotateZ(rotationDeg.z * kDegToRad), math::scale(scale)))));
}

static std::size_t resolveAnimatedFrameIndex(const render::AnimatedTexture& animatedTexture, double timeSeconds) {
  const std::size_t frameCount = animatedTexture.frames.size();
  if (frameCount == 0) return 0;

  const int startFrame = std::max(0, animatedTexture.startFrame);
  if (frameCount == 1 || animatedTexture.framesPerSecond <= 0.0001f) {
    return std::min<std::size_t>(static_cast<std::size_t>(startFrame), frameCount - 1);
  }

  const double frameValue = std::floor(std::max(0.0, timeSeconds * static_cast<double>(animatedTexture.framesPerSecond)));
  const std::size_t frameStep = static_cast<std::size_t>(frameValue) + static_cast<std::size_t>(startFrame);

  if (animatedTexture.pingPong && frameCount > 1) {
    const std::size_t cycle = frameCount * 2 - 2;
    std::size_t local = animatedTexture.looping ? (frameStep % cycle) : std::min(frameStep, cycle - 1);
    if (local >= frameCount) local = cycle - local;
    return std::min(local, frameCount - 1);
  }

  if (animatedTexture.looping) return frameStep % frameCount;
  if (frameStep >= frameCount) return animatedTexture.holdLastFrame ? (frameCount - 1) : 0;
  return frameStep;
}

static const render::AssetRef* resolveAnimatedTextureAsset(const render::AssetRef& baseTexture,
                                                           const render::AnimatedTexture* animatedTexture,
                                                           double timeSeconds) {
  if (!animatedTexture || !animatedTexture->enabled || animatedTexture->frames.empty()) {
    return (baseTexture.enabled && !baseTexture.key.empty()) ? &baseTexture : nullptr;
  }

  const render::AssetRef& frame = animatedTexture->frames[resolveAnimatedFrameIndex(*animatedTexture, timeSeconds)];
  if (frame.enabled && !frame.key.empty()) return &frame;
  return (baseTexture.enabled && !baseTexture.key.empty()) ? &baseTexture : nullptr;
}

static void ensureShadowMap(std::uint32_t& fbo, std::uint32_t& depthTex, int& curRes, int desiredRes) {
  desiredRes = std::max(128, std::min(4096, desiredRes));
  if (depthTex != 0 && fbo != 0 && curRes == desiredRes) return;

  if (fbo) glDeleteFramebuffers(1, &fbo);
  if (depthTex) glDeleteTextures(1, &depthTex);
  fbo = 0;
  depthTex = 0;
  curRes = desiredRes;

  glGenTextures(1, &depthTex);
  glBindTexture(GL_TEXTURE_2D, depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, desiredRes, desiredRes, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void ensureSceneTargets(std::uint32_t& fbo,
                               std::uint32_t& colorTex,
                               std::uint32_t& depthTex,
                               int& curW,
                               int& curH,
                               int desiredW,
                               int desiredH) {
  desiredW = std::max(1, desiredW);
  desiredH = std::max(1, desiredH);
  if (fbo != 0 && colorTex != 0 && depthTex != 0 && curW == desiredW && curH == desiredH) return;

  if (fbo) glDeleteFramebuffers(1, &fbo);
  if (colorTex) glDeleteTextures(1, &colorTex);
  if (depthTex) glDeleteTextures(1, &depthTex);
  fbo = 0;
  colorTex = 0;
  depthTex = 0;
  curW = desiredW;
  curH = desiredH;

  glGenTextures(1, &colorTex);
  glBindTexture(GL_TEXTURE_2D, colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, desiredW, desiredH, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenTextures(1, &depthTex);
  glBindTexture(GL_TEXTURE_2D, depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, desiredW, desiredH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
  const GLenum drawBuf = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &drawBuf);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void ensurePostTarget(std::uint32_t& fbo, std::uint32_t& colorTex, int& curW, int& curH, int desiredW, int desiredH) {
  desiredW = std::max(1, desiredW);
  desiredH = std::max(1, desiredH);
  if (fbo != 0 && colorTex != 0 && curW == desiredW && curH == desiredH) return;

  if (fbo) glDeleteFramebuffers(1, &fbo);
  if (colorTex) glDeleteTextures(1, &colorTex);
  fbo = 0;
  colorTex = 0;
  curW = desiredW;
  curH = desiredH;

  glGenTextures(1, &colorTex);
  glBindTexture(GL_TEXTURE_2D, colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, desiredW, desiredH, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
  const GLenum drawBuf = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &drawBuf);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

struct OverlayVert final {
  float x;
  float y;
  float r;
  float g;
  float b;
  float a;
};

struct DebugLineVert final {
  float x;
  float y;
  float z;
  float r;
  float g;
  float b;
  float a;
};

struct HudVert final {
  float x;
  float y;
  float z;
  float u;
  float v;
  float r;
  float g;
  float b;
  float a;
  float mode;
};

struct VfxVert final {
  float x;
  float y;
  float z;
  float r;
  float g;
  float b;
  float a;
  float size;
  float kind;
  float seed;
};

static float fract01(float value) {
  return value - std::floor(value);
}

static std::uint32_t hashU32(std::uint32_t value) {
  value ^= value >> 16;
  value *= 0x7feb352du;
  value ^= value >> 15;
  value *= 0x846ca68bu;
  value ^= value >> 16;
  return value;
}

static float hash01(std::uint32_t value) { return static_cast<float>(hashU32(value)) / 4294967295.0f; }

static math::Vec3 rotateVecDeg(const math::Vec3& v, const math::Vec3& rotationDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kDegToRad = kPi / 180.0f;
  const float rx = rotationDeg.x * kDegToRad;
  const float ry = rotationDeg.y * kDegToRad;
  const float rz = rotationDeg.z * kDegToRad;

  const float cx = std::cos(rx);
  const float sx = std::sin(rx);
  const float cy = std::cos(ry);
  const float sy = std::sin(ry);
  const float cz = std::cos(rz);
  const float sz = std::sin(rz);

  math::Vec3 out = v;
  out = {out.x, out.y * cx - out.z * sx, out.y * sx + out.z * cx};
  out = {out.x * cy + out.z * sy, out.y, -out.x * sy + out.z * cy};
  out = {out.x * cz - out.y * sz, out.x * sz + out.y * cz, out.z};
  return out;
}

static math::Vec3 vfxBasisRight(const math::Vec3& rotationDeg) { return math::normalize(rotateVecDeg({1.0f, 0.0f, 0.0f}, rotationDeg)); }
static math::Vec3 vfxBasisUp(const math::Vec3& rotationDeg) { return math::normalize(rotateVecDeg({0.0f, 1.0f, 0.0f}, rotationDeg)); }
static math::Vec3 vfxBasisForward(const math::Vec3& rotationDeg) { return math::normalize(rotateVecDeg({0.0f, 0.0f, 1.0f}, rotationDeg)); }

static float vfxQualityScale(ecs::VfxComponent::Quality quality) {
  switch (quality) {
    case ecs::VfxComponent::Quality::Low: return 0.45f;
    case ecs::VfxComponent::Quality::Medium: return 0.75f;
    case ecs::VfxComponent::Quality::High: return 1.0f;
    case ecs::VfxComponent::Quality::Ultra: return 1.4f;
  }
  return 1.0f;
}

static int vfxLODLevel(const ecs::systems::GraphicsSystem::FrameSnapshot::VfxDraw& vfx, float dist) {
  if (dist < vfx.lodForceNearDistance) return 3;
  if (dist < vfx.lodNearDistance) return 3;
  if (dist < vfx.lodMidDistance) return 2;
  if (dist < vfx.lodFarDistance) return 1;
  if (dist < vfx.lodUltraDistance) return 0;
  return 0;
}

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
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) { self->toggleCursorCaptured(); }
  if (key == GLFW_KEY_SPACE) {
    if (action == GLFW_PRESS && !self->m_keySpaceHeld) {
      self->m_keySpaceQueued = true;
      self->m_keySpaceHeld = true;
    } else if (action == GLFW_RELEASE) {
      self->m_keySpaceHeld = false;
    }
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

void OpenGlRenderer::applyCursorCaptured(bool captured) {
  if (!m_window) {
    m_cursorCaptured = captured;
    return;
  }
  m_cursorCaptured = captured;
  glfwSetInputMode(m_window, GLFW_CURSOR, m_cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, m_cursorCaptured ? GLFW_TRUE : GLFW_FALSE);
  }
  m_hasMousePos = false;
  m_accumMouseDx = 0.0;
  m_accumMouseDy = 0.0;
}

void OpenGlRenderer::setCursorCaptured(bool captured) { applyCursorCaptured(captured); }

void OpenGlRenderer::toggleCursorCaptured() { applyCursorCaptured(!m_cursorCaptured); }

int OpenGlRenderer::uniformLocation(std::uint32_t programId, const char* name) {
  if (programId == 0 || name == nullptr || *name == '\0') return -1;
  auto& perProgram = m_uniformLocationCache[programId];
  const auto it = perProgram.find(name);
  if (it != perProgram.end()) return it->second;
  const GLint location = glGetUniformLocation(programId, name);
  perProgram.emplace(name, location);
  return location;
}

std::uint32_t OpenGlRenderer::requestTextureAsset(const render::AssetRef& texture,
                                                  const render::AnimatedTexture* animatedTexture,
                                                  bool srgb,
                                                  double timeSeconds) {
  if (animatedTexture && animatedTexture->enabled) {
    for (const auto& frame : animatedTexture->frames) {
      if (frame.enabled && !frame.key.empty()) m_textures.requestTexture(frame.key, srgb);
    }
  }
  const render::AssetRef* selected = resolveAnimatedTextureAsset(texture, animatedTexture, timeSeconds);
  if (!selected || !selected->enabled || selected->key.empty()) return 0;
  return m_textures.requestTexture(selected->key, srgb);
}

void OpenGlRenderer::pollGpuTimerQueries() {
  if (!m_gpuTimerSupported) return;
  for (int i = 0; i < 2; ++i) {
    if (!m_gpuTimerPending[i] || m_gpuTimerQueries[i] == 0u) continue;
    GLuint available = 0u;
    glGetQueryObjectuiv(m_gpuTimerQueries[i], GL_QUERY_RESULT_AVAILABLE, &available);
    if (available != GL_TRUE) continue;
    GLuint64 elapsedNs = 0u;
    glGetQueryObjectui64v(m_gpuTimerQueries[i], GL_QUERY_RESULT, &elapsedNs);
    m_gpuTimerPending[i] = false;
    m_lastGpuFrameMs = static_cast<double>(elapsedNs) / 1000000.0;
    if (m_smoothedGpuFrameMs < 0.0) {
      m_smoothedGpuFrameMs = m_lastGpuFrameMs;
    } else {
      m_smoothedGpuFrameMs = (m_smoothedGpuFrameMs * 0.85) + (m_lastGpuFrameMs * 0.15);
    }
  }
}

void OpenGlRenderer::beginGpuTimerQuery() {
  if (!m_gpuTimerSupported || m_gpuTimerActive) return;
  const std::uint32_t queryId = m_gpuTimerQueries[m_gpuTimerWriteIndex];
  if (queryId == 0u) return;
  glBeginQuery(GL_TIME_ELAPSED, queryId);
  m_gpuTimerActive = true;
}

void OpenGlRenderer::endGpuTimerQuery() {
  if (!m_gpuTimerSupported || !m_gpuTimerActive) return;
  glEndQuery(GL_TIME_ELAPSED);
  m_gpuTimerPending[m_gpuTimerWriteIndex] = true;
  m_gpuTimerWriteIndex = (m_gpuTimerWriteIndex + 1) % 2;
  m_gpuTimerActive = false;
}

OpenGlRenderer::~OpenGlRenderer() { stop(); }

bool OpenGlRenderer::start(int width, int height, const char* title) {
  WindowConfig cfg{};
  cfg.width = width;
  cfg.height = height;
  return start(cfg, title);
}

bool OpenGlRenderer::start(const WindowConfig& cfg, const char* title) {
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

  GLFWmonitor* monitor = nullptr;
  int w = std::max(1, cfg.width);
  int h = std::max(1, cfg.height);
  if (cfg.fullscreen) {
    monitor = glfwGetPrimaryMonitor();
    if (monitor) {
      if (const GLFWvidmode* mode = glfwGetVideoMode(monitor)) {
        w = mode->width;
        h = mode->height;
        if (cfg.refreshRateHz > 0) {
          glfwWindowHint(GLFW_REFRESH_RATE, cfg.refreshRateHz);
        } else {
          glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        }
      }
    }
  }

  m_window = glfwCreateWindow(w, h, title, monitor, nullptr);
  if (!m_window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(cfg.vsync ? 1 : 0);
  glfwSetWindowUserPointer(m_window, this);
  glfwSetKeyCallback(m_window, &OpenGlRenderer::glfwKeyCallback);
  glfwSetCursorPosCallback(m_window, &OpenGlRenderer::glfwCursorPosCallback);

  // Capture mouse for FPS-style look by default.
  applyCursorCaptured(true);

  getFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
  glViewport(0, 0, m_fbWidth, m_fbHeight);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  m_textures.start();
  m_meshAssets.start();

  // Cache GPU strings (available after context creation).
  if (const auto* s = glGetString(GL_VENDOR)) m_gpuVendor = reinterpret_cast<const char*>(s);
  if (const auto* s = glGetString(GL_RENDERER)) m_gpuRenderer = reinterpret_cast<const char*>(s);
  if (const auto* s = glGetString(GL_VERSION)) m_glVersion = reinterpret_cast<const char*>(s);

  glGenQueries(2, m_gpuTimerQueries);
  m_gpuTimerSupported = (m_gpuTimerQueries[0] != 0u && m_gpuTimerQueries[1] != 0u);
  if (!m_gpuTimerSupported) {
    m_gpuTimerQueries[0] = 0u;
    m_gpuTimerQueries[1] = 0u;
  }
  m_gpuTimerPending[0] = false;
  m_gpuTimerPending[1] = false;
  m_gpuTimerWriteIndex = 0;
  m_gpuTimerActive = false;
  m_lastGpuFrameMs = -1.0;
  m_smoothedGpuFrameMs = -1.0;

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

  // World-space debug line program.
  {
    const std::string vsSrc =
        "#version 410 core\n"
        "layout(location=0) in vec3 a_Pos;\n"
        "layout(location=1) in vec4 a_Color;\n"
        "uniform mat4 u_ViewProj;\n"
        "out vec4 v_Color;\n"
        "void main(){ v_Color=a_Color; gl_Position=u_ViewProj*vec4(a_Pos,1.0); }\n";
    const std::string fsSrc =
        "#version 410 core\n"
        "in vec4 v_Color;\n"
        "out vec4 o_Color;\n"
        "void main(){ o_Color=v_Color; }\n";

    std::string err;
    const GLuint vs = compileGlShader(GL_VERTEX_SHADER, vsSrc, &err);
    if (!vs) {
      std::cerr << "Debug line vertex shader compile failed:\n" << err << "\n";
      return true;
    }
    const GLuint fs = compileGlShader(GL_FRAGMENT_SHADER, fsSrc, &err);
    if (!fs) {
      std::cerr << "Debug line fragment shader compile failed:\n" << err << "\n";
      glDeleteShader(vs);
      return true;
    }
    const GLuint prog = linkGlProgram(vs, fs, &err);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) {
      std::cerr << "Debug line program link failed:\n" << err << "\n";
      return true;
    }
    m_debugLineProgram = prog;

    glGenVertexArrays(1, &m_debugLineVao);
    glGenBuffers(1, &m_debugLineVbo);
    glBindVertexArray(m_debugLineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_debugLineVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugLineVert), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugLineVert), reinterpret_cast<void*>(sizeof(float) * 3));
    glBindVertexArray(0);
    m_debugLineCapacityVerts = 0;
  }

  // World-space VFX point-sprite program.
  {
    const std::string vsSrc =
        "#version 410 core\n"
        "layout(location=0) in vec3 a_Pos;\n"
        "layout(location=1) in vec4 a_Color;\n"
        "layout(location=2) in float a_Size;\n"
        "layout(location=3) in float a_Kind;\n"
        "layout(location=4) in float a_Seed;\n"
        "uniform mat4 u_ViewProj;\n"
        "uniform float u_PointScale;\n"
        "out vec4 v_Color;\n"
        "out float v_Kind;\n"
        "out float v_Seed;\n"
        "void main(){\n"
        "  vec4 clip = u_ViewProj * vec4(a_Pos, 1.0);\n"
        "  gl_Position = clip;\n"
        "  float depth = max(0.15, -clip.w);\n"
        "  gl_PointSize = clamp((a_Size * u_PointScale) / depth, 1.5, 140.0);\n"
        "  v_Color = a_Color;\n"
        "  v_Kind = a_Kind;\n"
        "  v_Seed = a_Seed;\n"
        "}\n";
    const std::string fsSrc =
        "#version 410 core\n"
        "in vec4 v_Color;\n"
        "in float v_Kind;\n"
        "in float v_Seed;\n"
        "uniform float u_Time;\n"
        "out vec4 o_Color;\n"
        "float hash11(float p){ return fract(sin(p * 91.7) * 43758.5453); }\n"
        "void main(){\n"
        "  vec2 uv = gl_PointCoord * 2.0 - 1.0;\n"
        "  float d = length(uv);\n"
        "  float alpha = smoothstep(1.0, 0.0, d);\n"
        "  vec3 col = v_Color.rgb;\n"
        "  if (v_Kind < 0.5) {\n"
        "    float flicker = 0.72 + 0.28 * sin(u_Time * 8.0 + v_Seed * 19.0);\n"
        "    float plume = smoothstep(1.0, -0.25, uv.y);\n"
        "    col = mix(col, vec3(1.0, 0.85, 0.35), 0.45 + 0.25 * flicker);\n"
        "    alpha *= plume * flicker;\n"
        "  } else if (v_Kind < 1.5) {\n"
        "    float sparkle = 0.65 + 0.35 * sin(u_Time * 24.0 + v_Seed * 31.0);\n"
        "    float core = smoothstep(0.55, 0.0, d);\n"
        "    col = mix(col, vec3(0.75, 0.9, 1.45), 0.55);\n"
        "    alpha *= core * sparkle;\n"
        "  } else if (v_Kind < 2.5) {\n"
        "    float glint = 0.75 + 0.25 * sin(u_Time * 16.0 + v_Seed * 11.0);\n"
        "    col = mix(col, vec3(1.0, 1.0, 0.9), 0.25);\n"
        "    alpha *= glint;\n"
        "  } else if (v_Kind < 3.5) {\n"
        "    col *= vec3(0.75, 0.78, 0.82);\n"
        "    alpha *= 0.65;\n"
        "  } else {\n"
        "    col *= vec3(0.85, 0.92, 1.0);\n"
        "    alpha *= 0.75;\n"
        "  }\n"
        "  if (alpha <= 0.01) discard;\n"
        "  o_Color = vec4(col, v_Color.a * alpha);\n"
        "}\n";

    std::string err;
    const GLuint vs = compileGlShader(GL_VERTEX_SHADER, vsSrc, &err);
    if (!vs) {
      std::cerr << "VFX vertex shader compile failed:\n" << err << "\n";
      return true;
    }
    const GLuint fs = compileGlShader(GL_FRAGMENT_SHADER, fsSrc, &err);
    if (!fs) {
      std::cerr << "VFX fragment shader compile failed:\n" << err << "\n";
      glDeleteShader(vs);
      return true;
    }
    const GLuint prog = linkGlProgram(vs, fs, &err);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) {
      std::cerr << "VFX program link failed:\n" << err << "\n";
      return true;
    }
    m_vfxProgram = prog;

    glGenVertexArrays(1, &m_vfxVao);
    glGenBuffers(1, &m_vfxVbo);
    glBindVertexArray(m_vfxVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vfxVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VfxVert), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VfxVert), reinterpret_cast<void*>(sizeof(float) * 3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(VfxVert), reinterpret_cast<void*>(sizeof(float) * 7));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(VfxVert), reinterpret_cast<void*>(sizeof(float) * 8));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(VfxVert), reinterpret_cast<void*>(sizeof(float) * 9));
    glBindVertexArray(0);
    m_vfxCapacityVerts = 0;
  }

  // HUD quad program (screen-space or world-space textured quads).
  {
    const std::string vsSrc =
        "#version 410 core\n"
        "layout(location=0) in vec3 a_Pos;\n"
        "layout(location=1) in vec2 a_Uv;\n"
        "layout(location=2) in vec4 a_Color;\n"
        "layout(location=3) in float a_Mode;\n"
        "uniform mat4 u_ViewProj;\n"
        "out vec2 v_Uv;\n"
        "out vec4 v_Color;\n"
        "flat out float v_Mode;\n"
        "void main(){\n"
        "  v_Uv = a_Uv;\n"
        "  v_Color = a_Color;\n"
        "  v_Mode = a_Mode;\n"
        "  gl_Position = (a_Mode < 0.5) ? vec4(a_Pos, 1.0) : (u_ViewProj * vec4(a_Pos, 1.0));\n"
        "}\n";
    const std::string fsSrc =
        "#version 410 core\n"
        "in vec2 v_Uv;\n"
        "in vec4 v_Color;\n"
        "flat in float v_Mode;\n"
        "uniform sampler2D u_Texture;\n"
        "uniform int u_UseTexture;\n"
        "out vec4 o_Color;\n"
        "void main(){\n"
        "  vec4 col = v_Color;\n"
        "  if (u_UseTexture != 0) col *= texture(u_Texture, v_Uv);\n"
        "  if (col.a <= 0.01) discard;\n"
        "  o_Color = col;\n"
        "}\n";

    std::string err;
    const GLuint vs = compileGlShader(GL_VERTEX_SHADER, vsSrc, &err);
    if (!vs) {
      std::cerr << "HUD vertex shader compile failed:\n" << err << "\n";
      return true;
    }
    const GLuint fs = compileGlShader(GL_FRAGMENT_SHADER, fsSrc, &err);
    if (!fs) {
      std::cerr << "HUD fragment shader compile failed:\n" << err << "\n";
      glDeleteShader(vs);
      return true;
    }
    const GLuint prog = linkGlProgram(vs, fs, &err);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) {
      std::cerr << "HUD program link failed:\n" << err << "\n";
      return true;
    }
    m_hudProgram = prog;

    glGenVertexArrays(1, &m_hudVao);
    glGenBuffers(1, &m_hudVbo);
    glBindVertexArray(m_hudVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_hudVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(HudVert), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(HudVert), reinterpret_cast<void*>(sizeof(float) * 3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(HudVert), reinterpret_cast<void*>(sizeof(float) * 5));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(HudVert), reinterpret_cast<void*>(sizeof(float) * 9));
    glBindVertexArray(0);
    m_hudCapacityVerts = 0;
  }

  // Sky VAO (core profile requires a VAO even for gl_VertexID fullscreen triangles).
  glGenVertexArrays(1, &m_skyVao);

  return true;
}

void OpenGlRenderer::stop() {
  m_shaders.clear();
  m_terrainLodState.clear();
  m_uniformLocationCache.clear();

  for (auto& [_, m] : m_terrainMeshes) {
    destroyTerrainMesh(m);
  }
  m_terrainMeshes.clear();

  for (auto& [_, m] : m_rockMeshes) {
    destroyRockMesh(m);
  }
  m_rockMeshes.clear();

  for (auto& [_, m] : m_grassMeshes) {
    destroyGrassMesh(m);
  }
  m_grassMeshes.clear();

  for (auto& [_, m] : m_gpuMeshes) {
    destroyGpuMesh(m);
  }
  m_gpuMeshes.clear();
  m_meshLogState.clear();

  if (m_window) {
    glfwMakeContextCurrent(m_window);
    if (m_gpuTimerActive) {
      glEndQuery(GL_TIME_ELAPSED);
      m_gpuTimerActive = false;
    }
    if (m_gpuTimerQueries[0] || m_gpuTimerQueries[1]) glDeleteQueries(2, m_gpuTimerQueries);
    if (m_skyVao) glDeleteVertexArrays(1, &m_skyVao);
    if (m_shadowFbo) glDeleteFramebuffers(1, &m_shadowFbo);
    if (m_shadowDepthTex) glDeleteTextures(1, &m_shadowDepthTex);
    if (m_sceneFbo) glDeleteFramebuffers(1, &m_sceneFbo);
    if (m_sceneColorTex) glDeleteTextures(1, &m_sceneColorTex);
    if (m_sceneDepthTex) glDeleteTextures(1, &m_sceneDepthTex);
    if (m_postFbo) glDeleteFramebuffers(1, &m_postFbo);
    if (m_postColorTex) glDeleteTextures(1, &m_postColorTex);
    m_shadowFbo = 0;
    m_shadowDepthTex = 0;
    m_shadowRes = 0;
    m_sceneFbo = 0;
    m_sceneColorTex = 0;
    m_sceneDepthTex = 0;
    m_sceneW = 0;
    m_sceneH = 0;
    m_postFbo = 0;
    m_postColorTex = 0;
    m_postW = 0;
    m_postH = 0;
    m_hasPrevViewProj = false;
    m_gpuTimerQueries[0] = 0u;
    m_gpuTimerQueries[1] = 0u;
    m_gpuTimerPending[0] = false;
    m_gpuTimerPending[1] = false;
    m_gpuTimerWriteIndex = 0;
    m_gpuTimerSupported = false;
    m_lastGpuFrameMs = -1.0;
    m_smoothedGpuFrameMs = -1.0;
    m_textures.destroyAllGlTextures();
    glfwMakeContextCurrent(nullptr);
  }
  m_textures.stop();
  m_meshAssets.stop();

  if (m_overlayVbo) glDeleteBuffers(1, &m_overlayVbo);
  if (m_overlayVao) glDeleteVertexArrays(1, &m_overlayVao);
  if (m_overlayProgram) glDeleteProgram(m_overlayProgram);
  if (m_hudVbo) glDeleteBuffers(1, &m_hudVbo);
  if (m_hudVao) glDeleteVertexArrays(1, &m_hudVao);
  if (m_hudProgram) glDeleteProgram(m_hudProgram);
  if (m_debugLineVbo) glDeleteBuffers(1, &m_debugLineVbo);
  if (m_debugLineVao) glDeleteVertexArrays(1, &m_debugLineVao);
  if (m_debugLineProgram) glDeleteProgram(m_debugLineProgram);
  if (m_vfxVbo) glDeleteBuffers(1, &m_vfxVbo);
  if (m_vfxVao) glDeleteVertexArrays(1, &m_vfxVao);
  if (m_vfxProgram) glDeleteProgram(m_vfxProgram);
  m_overlayVbo = 0;
  m_overlayVao = 0;
  m_overlayProgram = 0;
  m_overlayCapacityVerts = 0;
  m_hudVbo = 0;
  m_hudVao = 0;
  m_hudProgram = 0;
  m_hudCapacityVerts = 0;
  m_debugLineVbo = 0;
  m_debugLineVao = 0;
  m_debugLineProgram = 0;
  m_debugLineCapacityVerts = 0;
  m_vfxVbo = 0;
  m_vfxVao = 0;
  m_vfxProgram = 0;
  m_vfxCapacityVerts = 0;
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
  out.jump = m_keySpaceQueued;
  out.lookActive = m_cursorCaptured;
  out.mouseDx = static_cast<float>(m_accumMouseDx);
  out.mouseDy = static_cast<float>(m_accumMouseDy);
  m_accumMouseDx = 0.0;
  m_accumMouseDy = 0.0;
  m_keySpaceQueued = false;
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

void OpenGlRenderer::destroyGrassMesh(GrassMesh& m) {
  if (m.instanceVbo) glDeleteBuffers(1, &m.instanceVbo);
  if (m.ebo) glDeleteBuffers(1, &m.ebo);
  if (m.vbo) glDeleteBuffers(1, &m.vbo);
  if (m.vao) glDeleteVertexArrays(1, &m.vao);
  m = {};
}

void OpenGlRenderer::destroyGpuMesh(GpuMeshAsset& m) {
  for (auto& sm : m.subMeshes) {
    if (sm.ebo) glDeleteBuffers(1, &sm.ebo);
    if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
    if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
  }
  m = {};
}

OpenGlRenderer::GpuMeshAsset* OpenGlRenderer::getOrCreateGpuMesh(const std::string& path) {
  if (path.empty()) return nullptr;
  auto it = m_gpuMeshes.find(path);
  if (it != m_gpuMeshes.end() && it->second.ready) return &it->second;

  const auto status = m_meshAssets.request(path);
  auto& lastLogged = m_meshLogState[path];
  if (lastLogged != status.state) {
    lastLogged = status.state;
    if (status.state == assets::MeshAssetService::State::Queued) {
      std::cout << "Queued mesh asset: " << path << "\n";
    } else if (status.state == assets::MeshAssetService::State::Loading) {
      std::cout << "Loading mesh asset: " << path << "\n";
    } else if (status.state == assets::MeshAssetService::State::Failed) {
      std::cerr << "Mesh asset failed: " << path << " (" << status.error << ")\n";
    }
  }
  auto ready = m_meshAssets.takeReady(path);
  if (!ready) return nullptr;

  GpuMeshAsset gpu;
  gpu.ready = true;
  gpu.data = std::move(*ready);
  gpu.subMeshes.reserve(gpu.data.subMeshes.size());

  for (const auto& cpu : gpu.data.subMeshes) {
    GpuSubMesh sm;
    sm.indexCount = static_cast<std::uint32_t>(cpu.indices.size());
    sm.materialIndex = cpu.materialIndex;

    glGenVertexArrays(1, &sm.vao);
    glGenBuffers(1, &sm.vbo);
    glGenBuffers(1, &sm.ebo);

    glBindVertexArray(sm.vao);
    glBindBuffer(GL_ARRAY_BUFFER, sm.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(cpu.vertices.size() * sizeof(assets::MeshVertex)),
                 cpu.vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(cpu.indices.size() * sizeof(std::uint32_t)),
                 cpu.indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(assets::MeshVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(assets::MeshVertex), reinterpret_cast<void*>(sizeof(float) * 3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(assets::MeshVertex), reinterpret_cast<void*>(sizeof(float) * 6));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, sizeof(assets::MeshVertex), reinterpret_cast<void*>(sizeof(float) * 8));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(assets::MeshVertex),
                          reinterpret_cast<void*>(sizeof(float) * 8 + sizeof(std::uint16_t) * 4));
    glBindVertexArray(0);

    gpu.subMeshes.push_back(sm);
  }

  auto [ins, _] = m_gpuMeshes.emplace(path, std::move(gpu));
  return &ins->second;
}

static std::uint64_t terrainMeshKey(std::uint32_t id, int lodStep) {
  const std::uint64_t lod = static_cast<std::uint64_t>(std::max(1, std::min(256, lodStep)));
  return (static_cast<std::uint64_t>(id) << 8) | lod;
}

OpenGlRenderer::TerrainMesh* OpenGlRenderer::getOrCreateTerrainMesh(const ecs::systems::GraphicsSystem::TerrainDraw& t,
                                                                    int lodStep) {
  const std::uint32_t id = static_cast<std::uint32_t>(t.entity);
  lodStep = std::max(1, std::min(256, lodStep));

  const std::uint64_t key = terrainMeshKey(id, lodStep);
  auto it = m_terrainMeshes.find(key);
  if (it != m_terrainMeshes.end()) {
    auto& m = it->second;
    if (m.gridWidth == t.gridWidth && m.gridHeight == t.gridHeight && m.cellSizeMeters == t.cellSizeMeters &&
        m.heightScaleMeters == t.heightScaleMeters && m.noiseSeed == t.noiseSeed && m.lodStep == lodStep) {
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
  mesh.lodStep = lodStep;

  const int wCells = std::max(2, t.gridWidth);
  const int hCells = std::max(2, t.gridHeight);

  // LOD reduces vertex density but must keep the same world-space extents.
  // `lodStep` means "sample every Nth cell" (plus the last edge), not "shrink the terrain".
  const int step = std::max(1, lodStep);
  const int w = std::max(2, (wCells + step - 1) / step);
  const int h = std::max(2, (hCells + step - 1) / step);
  const int vertsW = w + 1;
  const int vertsH = h + 1;

  terrain::PerlinNoise2D noise(mesh.noiseSeed);

  const float fullHalfW = (static_cast<float>(wCells) * t.cellSizeMeters) * 0.5f;
  const float fullHalfH = (static_cast<float>(hCells) * t.cellSizeMeters) * 0.5f;

  std::vector<float> heights;
  heights.resize(static_cast<std::size_t>(vertsW * vertsH));
  for (int z = 0; z < vertsH; ++z) {
    for (int x = 0; x < vertsW; ++x) {
      const int cx = std::min(wCells, x * step);
      const int cz = std::min(hCells, z * step);
      const float worldX = t.position.x + (static_cast<float>(cx) * t.cellSizeMeters - fullHalfW);
      const float worldZ = t.position.z + (static_cast<float>(cz) * t.cellSizeMeters - fullHalfH);
      const float n = noise.sampleFractal(worldX, worldZ, t.noise);
      heights[static_cast<std::size_t>(z * vertsW + x)] = n * t.heightScaleMeters;
    }
  }

  std::vector<Vertex> vertices;
  vertices.resize(static_cast<std::size_t>(vertsW * vertsH));

  const float halfW = (static_cast<float>(wCells) * t.cellSizeMeters) * 0.5f;
  const float halfH = (static_cast<float>(hCells) * t.cellSizeMeters) * 0.5f;
  const float sampleSpacing = t.cellSizeMeters * static_cast<float>(step);

  auto heightAt = [&](int x, int z) -> float {
    x = std::max(0, std::min(vertsW - 1, x));
    z = std::max(0, std::min(vertsH - 1, z));
    return heights[static_cast<std::size_t>(z * vertsW + x)];
  };

  for (int z = 0; z < vertsH; ++z) {
    for (int x = 0; x < vertsW; ++x) {
      const int cx = std::min(wCells, x * step);
      const int cz = std::min(hCells, z * step);
      const float px = static_cast<float>(cx) * t.cellSizeMeters - halfW;
      const float pz = static_cast<float>(cz) * t.cellSizeMeters - halfH;
      const float py = heightAt(x, z);

      // Finite difference normal.
      const float hl = heightAt(x - 1, z);
      const float hr = heightAt(x + 1, z);
      const float hd = heightAt(x, z - 1);
      const float hu = heightAt(x, z + 1);
      const math::Vec3 n = math::normalize(math::Vec3{hl - hr, 2.0f * sampleSpacing, hd - hu});

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

  auto [insIt, _] = m_terrainMeshes.emplace(key, mesh);
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
      cb.center = {(x0 + x1) * 0.5f, 0.0f, (z0 + z1) * 0.5f};
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
    inst.px = x;
    inst.pz = z;

    float groundY = groundTerrain ? groundTerrain->position.y : r.position.y;
    if (groundHeightScale != 0.0f) {
      groundY += heightNoise.sampleFractal(worldX, worldZ, groundCfg) * groundHeightScale;
    }
    inst.py = groundY - r.position.y - 0.01f;

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
    c.centerLocal = cb.center;
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

OpenGlRenderer::GrassMesh* OpenGlRenderer::getOrCreateGrassMesh(
    const ecs::systems::GraphicsSystem::FrameSnapshot::GrassDraw& g,
    std::size_t layerIndex,
    const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain) {
  if (layerIndex >= g.layers.size()) return nullptr;
  const auto& layer = g.layers[layerIndex];

  const auto makeKey = [&](std::uint32_t entityId, std::size_t li) -> std::uint64_t {
    return (static_cast<std::uint64_t>(entityId) << 32) ^ static_cast<std::uint64_t>(li & 0xFFFFFFFFu);
  };

  const std::uint32_t entityId = static_cast<std::uint32_t>(g.entity);
  const std::uint64_t key = makeKey(entityId, layerIndex);
  auto it = m_grassMeshes.find(key);

  const float density = std::max(0.0f, g.densityMultiplier) * std::max(0.0f, layer.density);

  const bool useDensityMask = g.hasDensityMaskTex && g.densityMaskStrength > 0.0f && !g.densityMaskTex.key.empty();
  if (useDensityMask) m_textures.requestGreenMask(g.densityMaskTex.key);
  const bool densityMaskReady = useDensityMask ? m_textures.hasGreenMaskReady(g.densityMaskTex.key) : false;

  auto needsRebuild = [&](const GrassMesh& m) {
    return m.seed != g.seed || m.area.x != g.area.x || m.area.z != g.area.z || m.density != density || m.minScale != layer.minScale ||
           m.maxScale != layer.maxScale || m.bladeSpacing != layer.bladeSpacing || m.minSlopeDeg != layer.minSlopeDeg ||
           m.maxSlopeDeg != layer.maxSlopeDeg || m.bendStrength != layer.bendStrength || m.curveStrength != layer.curveStrength ||
           m.twistStrength != layer.twistStrength ||
           m.minAltitude != layer.minAltitude || m.maxAltitude != layer.maxAltitude || m.noiseScale != layer.noiseScale ||
           m.noiseStrength != layer.noiseStrength || m.species != layer.species ||
           m.densityNoise.frequency != g.densityNoise.frequency || m.densityNoise.octaves != g.densityNoise.octaves ||
           m.densityNoise.lacunarity != g.densityNoise.lacunarity || m.densityNoise.persistence != g.densityNoise.persistence ||
           m.densityNoise.seed != g.densityNoise.seed || m.densityNoiseThreshold != g.densityNoiseThreshold ||
           m.densityNoiseContrast != g.densityNoiseContrast || m.densityNoiseStrength != g.densityNoiseStrength ||
           m.islandNoise.frequency != g.islandNoise.frequency || m.islandNoise.octaves != g.islandNoise.octaves ||
           m.islandNoise.lacunarity != g.islandNoise.lacunarity || m.islandNoise.persistence != g.islandNoise.persistence ||
           m.islandNoise.seed != g.islandNoise.seed || m.islandNoiseOffset.x != g.islandNoiseOffset.x ||
           m.islandNoiseOffset.z != g.islandNoiseOffset.z || m.islandNoiseThreshold != g.islandNoiseThreshold ||
           m.islandNoiseSoftness != g.islandNoiseSoftness || m.islandNoiseContrast != g.islandNoiseContrast ||
           m.islandNoiseStrength != g.islandNoiseStrength ||
           m.densityMaskKey != (useDensityMask ? g.densityMaskTex.key : std::string{}) ||
           m.densityMaskStrength != g.densityMaskStrength || m.densityMaskTiling != g.densityMaskTiling ||
           m.densityMaskReady != densityMaskReady;
  };

  if (it != m_grassMeshes.end() && !needsRebuild(it->second)) return &it->second;

  if (it != m_grassMeshes.end()) {
    destroyGrassMesh(it->second);
    m_grassMeshes.erase(it);
  }

  GrassMesh mesh{};
  mesh.seed = g.seed;
  mesh.area = g.area;
  mesh.density = density;
  mesh.minScale = layer.minScale;
  mesh.maxScale = layer.maxScale;
  mesh.bladeSpacing = layer.bladeSpacing;
  mesh.bendStrength = layer.bendStrength;
  mesh.curveStrength = layer.curveStrength;
  mesh.twistStrength = layer.twistStrength;
  mesh.minSlopeDeg = layer.minSlopeDeg;
  mesh.maxSlopeDeg = layer.maxSlopeDeg;
  mesh.minAltitude = layer.minAltitude;
  mesh.maxAltitude = layer.maxAltitude;
  mesh.noiseScale = layer.noiseScale;
  mesh.noiseStrength = layer.noiseStrength;
  mesh.densityNoise = g.densityNoise;
  if (mesh.densityNoise.seed == 0u) mesh.densityNoise.seed = g.seed;
  mesh.densityNoiseThreshold = g.densityNoiseThreshold;
  mesh.densityNoiseContrast = g.densityNoiseContrast;
  mesh.densityNoiseStrength = g.densityNoiseStrength;
  mesh.islandNoise = g.islandNoise;
  if (mesh.islandNoise.seed == 0u) mesh.islandNoise.seed = g.seed ^ 0x7F4A7C15u;
  mesh.islandNoiseOffset = g.islandNoiseOffset;
  mesh.islandNoiseThreshold = g.islandNoiseThreshold;
  mesh.islandNoiseSoftness = g.islandNoiseSoftness;
  mesh.islandNoiseContrast = g.islandNoiseContrast;
  mesh.islandNoiseStrength = g.islandNoiseStrength;
  mesh.species = layer.species;
  mesh.densityMaskKey = useDensityMask ? g.densityMaskTex.key : std::string{};
  mesh.densityMaskStrength = g.densityMaskStrength;
  mesh.densityMaskTiling = g.densityMaskTiling;
  mesh.densityMaskReady = densityMaskReady;

  struct GrassVert {
    float px, py, pz;
    float u, v;
    float planeId;
  };
  struct GrassInstance {
    float px, py, pz;
    float scale;
    float rot;
    float var;
  };

  // Build modest carpet cards. The shader turns these into thick hazy grass mass,
  // avoiding a brute-force blade count.
  int bladeCount = 10;
  int segments = 5;
  float baseWidth = 0.055f;
  float radialSpread = 0.080f;
  float bladeHeightMin = 0.70f;
  float bladeHeightMax = 1.00f;
  float leanStrength = 0.08f;
  if (mesh.species == "BillboardGrassPlanes") {
    bladeCount = 3;
    segments = 3;
    baseWidth = 0.42f;
    radialSpread = 0.06f;
    bladeHeightMin = 0.42f;
    bladeHeightMax = 0.74f;
    leanStrength = 0.06f;
  } else if (mesh.species == "ShaderGrassCarpet") {
    bladeCount = 4;
    segments = 3;
    baseWidth = 0.30f;
    radialSpread = 0.36f;
    bladeHeightMin = 0.26f;
    bladeHeightMax = 0.56f;
    leanStrength = 0.10f;
  } else if (mesh.species == "GroundCover") {
    bladeCount = 16;
    segments = 4;
    baseWidth = 0.052f;
    radialSpread = 0.110f;
    bladeHeightMin = 0.34f;
    bladeHeightMax = 0.76f;
    leanStrength = 0.075f;
  } else if (mesh.species == "TallGrass") {
    bladeCount = 18;
    segments = 5;
    baseWidth = 0.058f;
    radialSpread = 0.140f;
    bladeHeightMin = 0.58f;
    bladeHeightMax = 1.06f;
    leanStrength = 0.16f;
  } else if (mesh.species == "BroadLeafGrass") {
    bladeCount = 11;
    segments = 4;
    baseWidth = 0.105f;
    radialSpread = 0.120f;
    bladeHeightMin = 0.44f;
    bladeHeightMax = 0.88f;
    leanStrength = 0.13f;
  } else if (mesh.species == "DryGrass") {
    bladeCount = 14;
    segments = 5;
    baseWidth = 0.050f;
    radialSpread = 0.130f;
    bladeHeightMin = 0.58f;
    bladeHeightMax = 0.88f;
    leanStrength = 0.12f;
  } else if (mesh.species == "Weed") {
    bladeCount = 13;
    segments = 5;
    baseWidth = 0.070f;
    radialSpread = 0.130f;
    bladeHeightMin = 0.58f;
    bladeHeightMax = 0.94f;
    leanStrength = 0.10f;
  } else if (mesh.species == "SmallFlower") {
    bladeCount = 6;
    segments = 3;
    baseWidth = 0.080f;
    radialSpread = 0.070f;
    bladeHeightMin = 0.62f;
    bladeHeightMax = 0.90f;
    leanStrength = 0.08f;
  }
  radialSpread *= std::max(0.2f, layer.bladeSpacing);

  auto hashStr = [](const std::string& s) -> std::uint32_t {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s) {
      h ^= static_cast<std::uint32_t>(c);
      h *= 16777619u;
    }
    return h;
  };

  const std::uint32_t geoSeed = hashStr(mesh.species) ^ 0xB5297A4Du;

  auto rand01 = [&](std::uint32_t n) {
    n ^= n >> 16;
    n *= 0x7feb352dU;
    n ^= n >> 15;
    n *= 0x846ca68bU;
    n ^= n >> 16;
    return (n & 0x00FFFFFFu) / 16777216.0f;
  };

  std::vector<GrassVert> verts;
  std::vector<std::uint32_t> idxs;
  verts.reserve(static_cast<std::size_t>(bladeCount) * static_cast<std::size_t>(segments + 1) * 2u);
  idxs.reserve(static_cast<std::size_t>(bladeCount) * static_cast<std::size_t>(segments) * 6u);

  const float twoPi = 6.2831853f;
  if (mesh.species == "BillboardGrassPlanes") {
    for (int p = 0; p < bladeCount; ++p) {
      const float planeJitter = rand01(geoSeed + static_cast<std::uint32_t>(p) * 26699u) * 2.0f - 1.0f;
      const auto spec = GrassBillboardFactory::planeSpec(p, planeJitter);
      const float localHeight = bladeHeightMin + (bladeHeightMax - bladeHeightMin) *
                                                  rand01(geoSeed + static_cast<std::uint32_t>(p) * 42437u) * spec.heightMul;
      const float localWidth = baseWidth * (0.82f + 0.28f * rand01(geoSeed + static_cast<std::uint32_t>(p) * 97531u));
      const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
      for (int i = 0; i <= segments; ++i) {
        const float v = static_cast<float>(i) / std::max(1.0f, static_cast<float>(segments));
        const float taper = std::pow(1.0f - v, 1.05f);
        const float h = localHeight * v;
        const float halfW = localWidth * (0.40f + 0.26f * taper);
        const float bendForward = (p == 2) ? std::pow(v, 1.65f) * (0.07f + 0.04f * rand01(geoSeed + 991u)) : 0.0f;
        verts.push_back(GrassVert{-halfW + spec.offset.x, h, bendForward + spec.offset.z, 0.0f, v, static_cast<float>(p)});
        verts.push_back(GrassVert{halfW + spec.offset.x, h, bendForward + spec.offset.z, 1.0f, v, static_cast<float>(p)});
      }
      for (int i = 0; i < segments; ++i) {
        const std::uint32_t i0 = base + static_cast<std::uint32_t>(i * 2 + 0);
        const std::uint32_t i1 = base + static_cast<std::uint32_t>(i * 2 + 1);
        const std::uint32_t i2 = base + static_cast<std::uint32_t>((i + 1) * 2 + 0);
        const std::uint32_t i3 = base + static_cast<std::uint32_t>((i + 1) * 2 + 1);
        idxs.push_back(i0);
        idxs.push_back(i2);
        idxs.push_back(i1);
        idxs.push_back(i1);
        idxs.push_back(i2);
        idxs.push_back(i3);
      }
    }
  } else for (int b = 0; b < bladeCount; ++b) {
    const float rb = rand01(geoSeed + static_cast<std::uint32_t>(b) * 2654435761u);
    const float yaw = (static_cast<float>(b) / std::max(1.0f, static_cast<float>(bladeCount))) * twoPi + (rb - 0.5f) * 0.90f;
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);
    const float w = baseWidth * (0.65f + 0.85f * rand01(geoSeed + static_cast<std::uint32_t>(b) * 97531u));
    const float offsetYaw = rand01(geoSeed + static_cast<std::uint32_t>(b) * 53189u) * twoPi;
    const float ro = radialSpread * std::sqrt(rand01(geoSeed + static_cast<std::uint32_t>(b) * 71237u));
    const float bladeHeight =
        bladeHeightMin + (bladeHeightMax - bladeHeightMin) * rand01(geoSeed + static_cast<std::uint32_t>(b) * 42437u);
    const float leanYaw = rand01(geoSeed + static_cast<std::uint32_t>(b) * 17713u) * twoPi;
    const float leanAmount = leanStrength * (0.35f + 0.65f * rand01(geoSeed + static_cast<std::uint32_t>(b) * 18149u));
    const float leanX = std::cos(leanYaw) * leanAmount;
    const float leanZ = std::sin(leanYaw) * leanAmount;
    const float curl = (rand01(geoSeed + static_cast<std::uint32_t>(b) * 22013u) - 0.5f) * 0.06f;
    const float ox = std::cos(offsetYaw) * ro;
    const float oz = std::sin(offsetYaw) * ro;
    const float heightJitter = (rand01(geoSeed + static_cast<std::uint32_t>(b) * 32257u) - 0.5f) * 0.035f;

    const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
    for (int i = 0; i <= segments; ++i) {
      const float v = static_cast<float>(i) / std::max(1.0f, static_cast<float>(segments));
      const float taper = std::pow(1.0f - v, 1.35f);
      const float shoulder = std::sin(v * 3.14159f);
      const float carpetShoulder = (mesh.species == "ShaderGrassCarpet") ? 0.18f : 0.10f;
      float ww = w * (0.14f + 0.78f * taper + carpetShoulder * shoulder);
      if (mesh.species == "ShaderGrassCarpet") {
        const float tipSpread = v * v * (0.36f + 0.18f * rand01(geoSeed + static_cast<std::uint32_t>(b) * 39419u));
        ww = w * (0.42f + 0.22f * taper + 0.22f * shoulder + tipSpread);
      }
      const float h = std::max(0.0f, bladeHeight * v + heightJitter * shoulder);
      const float bend = h * h * (0.52f + curl);

      float lx0 = -0.5f * ww;
      float lx1 = 0.5f * ww;
      if (mesh.species == "ShaderGrassCarpet") {
        const float fan = (v * v) * (0.035f + 0.045f * rand01(geoSeed + static_cast<std::uint32_t>(b) * 61543u));
        lx0 -= fan;
        lx1 += fan;
      }
      const float y = h;

      // Rotate around Y.
      const float x0 = c * lx0;
      const float z0 = s * lx0;
      const float x1 = c * lx1;
      const float z1 = s * lx1;
      const float bendX = leanX * bend;
      const float bendZ = leanZ * bend;

      verts.push_back(GrassVert{x0 + ox + bendX, y, z0 + oz + bendZ, 0.0f, v, 0.0f});
      verts.push_back(GrassVert{x1 + ox + bendX, y, z1 + oz + bendZ, 1.0f, v, 0.0f});
    }

    for (int i = 0; i < segments; ++i) {
      const std::uint32_t i0 = base + static_cast<std::uint32_t>(i * 2 + 0);
      const std::uint32_t i1 = base + static_cast<std::uint32_t>(i * 2 + 1);
      const std::uint32_t i2 = base + static_cast<std::uint32_t>((i + 1) * 2 + 0);
      const std::uint32_t i3 = base + static_cast<std::uint32_t>((i + 1) * 2 + 1);
      idxs.push_back(i0);
      idxs.push_back(i2);
      idxs.push_back(i1);
      idxs.push_back(i1);
      idxs.push_back(i2);
      idxs.push_back(i3);
    }
  }

  mesh.indexCount = static_cast<std::uint32_t>(idxs.size());

  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);
  glGenBuffers(1, &mesh.instanceVbo);

  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(GrassVert)), verts.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(idxs.size() * sizeof(std::uint32_t)), idxs.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GrassVert), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GrassVert), reinterpret_cast<void*>(sizeof(float) * 3));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(GrassVert), reinterpret_cast<void*>(sizeof(float) * 5));

  glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVbo);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GrassInstance), reinterpret_cast<void*>(0));
  glVertexAttribDivisor(3, 1);
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(GrassInstance), reinterpret_cast<void*>(sizeof(float) * 3));
  glVertexAttribDivisor(4, 1);
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GrassInstance), reinterpret_cast<void*>(sizeof(float) * 4));
  glVertexAttribDivisor(5, 1);
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(GrassInstance), reinterpret_cast<void*>(sizeof(float) * 5));
  glVertexAttribDivisor(6, 1);

  glBindVertexArray(0);

  // Instance generation (noisy density + slope/altitude masks).
  const float areaM2 = std::max(0.0f, g.area.x) * std::max(0.0f, g.area.z);
  const std::uint32_t maxInstances = static_cast<std::uint32_t>(
      std::min(60000.0f, std::max(0.0f, areaM2 * std::max(0.0f, density))));

  terrain::PerlinNoise2D densityNoise(g.seed ^ (0x9E3779B9u + static_cast<std::uint32_t>(layerIndex) * 1013u));
  terrain::PerlinNoise2D heightNoise(groundTerrain ? groundTerrain->noiseSeed : g.seed);
  const terrain::NoiseConfig groundCfg = groundTerrain ? groundTerrain->noise : terrain::NoiseConfig{};
  const float groundHeightScale = groundTerrain ? groundTerrain->heightScaleMeters : 0.0f;

  auto sampleGroundY = [&](float worldX, float worldZ) -> float {
    float groundY = groundTerrain ? groundTerrain->position.y : g.position.y;
    if (!groundTerrain || groundHeightScale == 0.0f) return groundY;
    groundY += heightNoise.sampleFractal(worldX, worldZ, groundCfg) * groundHeightScale;
    return groundY;
  };

  auto sampleSlopeDeg = [&](float worldX, float worldZ) -> float {
    const float eps = 0.35f;
    const float hL = sampleGroundY(worldX - eps, worldZ);
    const float hR = sampleGroundY(worldX + eps, worldZ);
    const float hD = sampleGroundY(worldX, worldZ - eps);
    const float hU = sampleGroundY(worldX, worldZ + eps);
    const float dhdx = (hR - hL) / (2.0f * eps);
    const float dhdz = (hU - hD) / (2.0f * eps);
    const float slopeRad = std::atan(std::sqrt(dhdx * dhdx + dhdz * dhdz));
    return slopeRad * 57.2957795f;
  };

  terrain::NoiseConfig densCfg;
  densCfg.frequency = std::max(0.005f, layer.noiseScale);
  densCfg.octaves = 2;
  densCfg.persistence = 0.55f;
  densCfg.lacunarity = 2.0f;
  terrain::NoiseConfig densityMaskCfg = g.densityNoise;
  if (densityMaskCfg.frequency <= 0.0f) densityMaskCfg.frequency = 0.03f;
  if (densityMaskCfg.octaves <= 0) densityMaskCfg.octaves = 1;
  if (densityMaskCfg.persistence <= 0.0f) densityMaskCfg.persistence = 0.55f;
  if (densityMaskCfg.lacunarity <= 0.0f) densityMaskCfg.lacunarity = 2.0f;
  densityMaskCfg.seed = g.densityNoise.seed != 0u ? g.densityNoise.seed : g.seed;
  terrain::PerlinNoise2D islandNoise(mesh.islandNoise.seed);
  terrain::NoiseConfig islandCfg = mesh.islandNoise;
  if (islandCfg.frequency <= 0.0f) islandCfg.frequency = 0.045f;
  if (islandCfg.octaves <= 0) islandCfg.octaves = 1;
  if (islandCfg.persistence <= 0.0f) islandCfg.persistence = 0.58f;
  if (islandCfg.lacunarity <= 0.0f) islandCfg.lacunarity = 2.0f;

  const float chunkSize = std::max(2.0f, mesh.chunkSizeMeters);
  const int chunkCountX = std::max(1, static_cast<int>(std::ceil(std::max(0.0f, g.area.x) / chunkSize)));
  const int chunkCountZ = std::max(1, static_cast<int>(std::ceil(std::max(0.0f, g.area.z) / chunkSize)));
  const int chunkCount = chunkCountX * chunkCountZ;
  const float halfW = g.area.x * 0.5f;
  const float halfD = g.area.z * 0.5f;

  struct ChunkBuild final {
    math::Vec3 center{};
    float radius = 0.0f;
    std::vector<GrassInstance> instances;
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
      cb.center = {(x0 + x1) * 0.5f, 0.0f, (z0 + z1) * 0.5f};
      const float rx = (x1 - x0) * 0.5f;
      const float rz = (z1 - z0) * 0.5f;
      cb.radius = std::sqrt(rx * rx + rz * rz) + 1.6f;
      cb.instances.reserve(static_cast<std::size_t>(maxInstances / std::max(1, chunkCount)));
    }
  }

  const std::uint32_t seed = g.seed ^ (static_cast<std::uint32_t>(layerIndex) * 0x85EBCA6Bu);

  struct Cluster final {
    float x = 0.0f;
    float z = 0.0f;
    float radius = 1.0f;
    float strength = 1.0f;
  };

  int clusterCount = std::clamp(static_cast<int>(std::ceil(areaM2 / 300.0f)), 3, 8);
  // Billboard planes should read like a more even carpet (islands/noise do the shaping).
  if (mesh.species == "BillboardGrassPlanes") clusterCount = 0;
  std::vector<Cluster> clusters;
  clusters.reserve(static_cast<std::size_t>(clusterCount));
  for (int i = 0; i < clusterCount; ++i) {
    const std::uint32_t ci = seed ^ (0x9E3779B9u + static_cast<std::uint32_t>(i) * 2246822519u);
    Cluster c{};
    c.x = (rand01(ci + 11u) * 2.0f - 1.0f) * halfW;
    c.z = (rand01(ci + 29u) * 2.0f - 1.0f) * halfD;
    const float minRadius = std::max(2.5f, std::min(halfW, halfD) * 0.16f);
    const float maxRadius = std::max(minRadius + 0.1f, std::min(halfW, halfD) * 0.36f);
    c.radius = minRadius + (maxRadius - minRadius) * rand01(ci + 47u);
    c.strength = 0.70f + 0.55f * rand01(ci + 71u);
    clusters.push_back(c);
  }

  float clusterBlend = 0.40f + 0.35f * std::clamp(layer.noiseStrength, 0.0f, 1.0f);
  float clusterSharpness = 1.35f + 1.75f * std::clamp(layer.noiseStrength, 0.0f, 1.0f);
  if (mesh.species == "ShaderGrassCarpet") clusterSharpness *= 0.62f;
  if (mesh.species == "BillboardGrassPlanes") clusterBlend = 0.0f;

  std::uint32_t attempts = 0;
  const std::uint32_t maxAttempts = maxInstances * 12u + 2048u;
  std::size_t generated = 0;

  while (generated < static_cast<std::size_t>(maxInstances) && attempts < maxAttempts) {
    const std::uint32_t idx = attempts++;
    const float rx = rand01(seed + idx * 9781u);
    const float rz = rand01(seed + idx * 6271u);
    const float rScale = rand01(seed + idx * 3137u);
    const float rRot = rand01(seed + idx * 1951u);
    const float rVar = rand01(seed + idx * 8111u);

    const float x = (rx * 2.0f - 1.0f) * halfW;
    const float z = (rz * 2.0f - 1.0f) * halfD;

    const float worldX = g.position.x + x;
    const float worldZ = g.position.z + z;

    const float n = (densityNoise.sampleFractal(worldX, worldZ, densCfg) + 1.0f) * 0.5f;
    const float densityMaskRaw = (densityNoise.sampleFractal(worldX, worldZ, densityMaskCfg) + 1.0f) * 0.5f;
    const float densityMaskContrast = std::max(0.1f, g.densityNoiseContrast);
    const float densityMaskThreshold = g.densityNoiseThreshold;
    float densityMask =
        std::clamp((densityMaskRaw - densityMaskThreshold) / std::max(0.0001f, 1.0f - densityMaskThreshold), 0.0f, 1.0f);
    densityMask = std::pow(densityMask, densityMaskContrast) * std::clamp(g.densityNoiseStrength, 0.0f, 1.0f);
    const float islandRaw =
        (islandNoise.sampleFractal(worldX + mesh.islandNoiseOffset.x, worldZ + mesh.islandNoiseOffset.z, islandCfg) + 1.0f) * 0.5f;
    const float islandSoft = std::max(0.0001f, mesh.islandNoiseSoftness);
    float islandMask = std::clamp((islandRaw - (mesh.islandNoiseThreshold - islandSoft)) / (2.0f * islandSoft), 0.0f, 1.0f);
    islandMask = islandMask * islandMask * (3.0f - 2.0f * islandMask);
    islandMask = std::pow(std::clamp(islandMask, 0.0f, 1.0f), std::max(0.1f, mesh.islandNoiseContrast));
    islandMask = (1.0f - std::clamp(mesh.islandNoiseStrength, 0.0f, 1.0f)) +
                 islandMask * std::clamp(mesh.islandNoiseStrength, 0.0f, 1.0f);
    float clusterMask = 0.0f;
    for (const auto& c : clusters) {
      const float dx = x - c.x;
      const float dz = z - c.z;
      const float dist = std::sqrt(dx * dx + dz * dz);
      const float falloff = std::clamp(1.0f - (dist / std::max(0.001f, c.radius)), 0.0f, 1.0f);
      clusterMask = std::max(clusterMask, c.strength * std::pow(falloff, clusterSharpness));
    }
    clusterMask = std::clamp(clusterMask, 0.0f, 1.0f);

    const float ns = std::clamp(layer.noiseStrength, 0.0f, 1.0f);
    const float noiseMask = (ns <= 0.0001f) ? 1.0f : std::clamp((n - (1.0f - ns)) / ns, 0.0f, 1.0f);
    float t =
        std::clamp((clusterMask * clusterBlend + noiseMask * (1.0f - clusterBlend)) * densityMask * islandMask, 0.0f, 1.0f);

    if (useDensityMask && densityMaskReady) {
      const float invAx = 1.0f / std::max(0.001f, g.area.x);
      const float invAz = 1.0f / std::max(0.001f, g.area.z);
      const float u = (x * invAx + 0.5f) * std::max(0.001f, g.densityMaskTiling);
      const float v = (z * invAz + 0.5f) * std::max(0.001f, g.densityMaskTiling);
      const float mask = m_textures.sampleGreenMask(g.densityMaskTex.key, u, v).value_or(0.0f);
      t = std::clamp(t * (1.0f + std::max(0.0f, g.densityMaskStrength) * mask), 0.0f, 1.0f);
    }

    if (rand01(seed ^ (idx * 1013904223u)) > t) continue;

    const float groundY = sampleGroundY(worldX, worldZ);
    if (groundY < layer.minAltitude || groundY > layer.maxAltitude) continue;

    const float slopeDeg = sampleSlopeDeg(worldX, worldZ);
    if (slopeDeg < layer.minSlopeDeg || slopeDeg > layer.maxSlopeDeg) continue;

    GrassInstance inst{};
    inst.px = x;
    inst.py = groundY - g.position.y;
    inst.pz = z;
    inst.scale = layer.minScale + (layer.maxScale - layer.minScale) * std::pow(rScale, 1.8f);
    inst.rot = rRot * twoPi;
    inst.var = rVar;

    const int cx = std::clamp(static_cast<int>((x + halfW) / chunkSize), 0, chunkCountX - 1);
    const int cz = std::clamp(static_cast<int>((z + halfD) / chunkSize), 0, chunkCountZ - 1);
    chunkBuilds[static_cast<std::size_t>(cz * chunkCountX + cx)].instances.push_back(inst);
    generated += 1;
  }

  std::vector<GrassInstance> allInstances;
  allInstances.reserve(maxInstances);
  mesh.chunks.clear();
  mesh.chunks.reserve(chunkBuilds.size());

  for (const auto& cb : chunkBuilds) {
    if (cb.instances.empty()) continue;
    GrassMesh::Chunk c;
    c.centerLocal = cb.center;
    c.radius = cb.radius;
    c.instanceOffset = static_cast<std::uint32_t>(allInstances.size());
    c.instanceCount = static_cast<std::uint32_t>(cb.instances.size());
    allInstances.insert(allInstances.end(), cb.instances.begin(), cb.instances.end());
    mesh.chunks.push_back(c);
  }

  mesh.instanceCapacity = static_cast<std::uint32_t>(allInstances.size());
  glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(allInstances.size() * sizeof(GrassInstance)),
               allInstances.empty() ? nullptr : allInstances.data(), GL_DYNAMIC_DRAW);

  auto [insIt, _] = m_grassMeshes.emplace(key, mesh);
  return &insIt->second;
}

void OpenGlRenderer::render(const ecs::systems::GraphicsSystem::FrameSnapshot& frame,
                            bool debugHudEnabled,
                            float fpsEstimate,
                            float deltaMs,
                            float cpuWorkMs,
                            const std::string& extraDebugText) {
  if (!m_window) return;

  const double renderStart = glfwGetTime();
  m_renderDebugger.beginFrame();
  pollGpuTimerQueries();
  beginGpuTimerQuery();
#define glGetUniformLocation(PROGRAM, NAME) uniformLocation((PROGRAM), (NAME))
  auto recordPass = [&](const char* label, double startSeconds) {
    m_renderDebugger.record(label, (glfwGetTime() - startSeconds) * 1000.0);
  };

  double passStart = glfwGetTime();
  m_textures.flushUploads(4);

  const float rs = std::clamp(frame.camera.renderScale, 0.25f, 1.0f);
  const int sceneW = std::max(1, static_cast<int>(static_cast<float>(m_fbWidth) * rs));
  const int sceneH = std::max(1, static_cast<int>(static_cast<float>(m_fbHeight) * rs));
  ensureSceneTargets(m_sceneFbo, m_sceneColorTex, m_sceneDepthTex, m_sceneW, m_sceneH, sceneW, sceneH);

  const std::uint32_t mainFbo = m_sceneFbo ? m_sceneFbo : 0u;
  glBindFramebuffer(GL_FRAMEBUFFER, mainFbo);
  glViewport(0, 0, sceneW, sceneH);

  glClearColor(0.55f, 0.72f, 0.92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float aspect = frame.camera.useFramebufferAspectRatio ? (static_cast<float>(sceneW) / static_cast<float>(sceneH))
                                                              : std::max(0.001f, frame.camera.aspectRatio);
  const bool isOrtho = (frame.camera.projectionType != 0);
  math::Mat4 proj{};
  if (isOrtho) {
    const float halfH = std::max(0.01f, frame.camera.orthographicSize);
    const float halfW = halfH * aspect;
    proj = ortho(-halfW, halfW, -halfH, halfH, frame.camera.nearClip, frame.camera.farClip);
  } else {
    const float fovRad = frame.camera.fovYRadians;
    proj = math::perspective(fovRad, aspect, frame.camera.nearClip, frame.camera.farClip);
  }
  const math::Mat4 view =
      math::lookAt(frame.camera.position, frame.camera.position + frame.camera.forward, math::Vec3{0.0f, 1.0f, 0.0f});
  const math::Mat4 viewProj = math::mul(proj, view);
  const math::Mat4 invViewProj = math::inverse(viewProj);

  const math::Vec3 camPos = frame.camera.position;
  const math::Vec3 camFwd = math::normalize(frame.camera.forward);
  const math::Vec3 camFwdXZ = math::normalize(math::Vec3{camFwd.x, 0.0f, camFwd.z});

  const auto mulPoint = [](const math::Mat4& m, const math::Vec3& p) -> math::Vec3 {
    // Column-major: out = M * vec4(p,1)
    return math::Vec3{
        m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
        m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
        m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
    };
  };

  const auto clampf = [](float v, float lo, float hi) -> float { return (v < lo) ? lo : (v > hi) ? hi : v; };
  recordPass("setup", passStart);

  // --- Terrain LOD state update (shared by shadow + main passes) ---
  passStart = glfwGetTime();
  for (const auto& t : frame.terrains) {
    auto& st = m_terrainLodState[static_cast<std::uint32_t>(t.entity)];
    if (st.lodStep <= 0) st.lodStep = 1;

    // Distance-to-terrain for LOD: use nearest point on the terrain XZ rectangle (not the terrain center).
    const float sizeX = static_cast<float>(std::max(2, t.gridWidth)) * t.cellSizeMeters;
    const float sizeZ = static_cast<float>(std::max(2, t.gridHeight)) * t.cellSizeMeters;
    const float halfX = 0.5f * sizeX;
    const float halfZ = 0.5f * sizeZ;

    const float nearestX = clampf(camPos.x, t.position.x - halfX, t.position.x + halfX);
    const float nearestZ = clampf(camPos.z, t.position.z - halfZ, t.position.z + halfZ);
    const float dx = nearestX - camPos.x;
    const float dz = nearestZ - camPos.z;
    const float distHoriz = std::sqrt(dx * dx + dz * dz);

    float viewDot = 1.0f;
    if (distHoriz > 1e-4f) {
      const float inv = 1.0f / distHoriz;
      const math::Vec3 dirXZ{dx * inv, 0.0f, dz * inv};
      // Use yaw-only dot so pitching the camera doesn't cause popping.
      viewDot = (camFwdXZ.x * dirXZ.x + camFwdXZ.z * dirXZ.z);
    }

    if (distHoriz > t.lodMaxRenderDistance) {
      st.wantTess = false;
      st.lodStep = std::max(st.lodStep, 16);
      continue;
    }

    // --- Tessellation decision (hysteresis) ---
    const bool tessAllowed = (t.tessQuality > 0);
    const float lockDist = std::max(10.0f, t.tessLockDistance);  // never disable right in front of the camera
    const float enableDist = t.tessEnableDistance;
    const float disableDist = std::max(enableDist + 1.0f, t.tessDisableDistance);
    const float enableDot = t.viewDotBias + 0.10f;
    const float disableDot = std::max(0.0f, t.viewDotBias - 0.03f);

    if (!tessAllowed) {
      st.wantTess = false;
    } else if (distHoriz < lockDist) {
      st.wantTess = true;
    } else if (st.wantTess) {
      if (distHoriz > disableDist || viewDot < disableDot) st.wantTess = false;
    } else {
      if (distHoriz < enableDist && viewDot > enableDot) st.wantTess = true;
    }

    // --- Geometry LOD (hysteresis) ---
    const float t12_in = t.lodStep1Distance;
    const float t12_out = std::max(1.0f, t12_in - 6.0f);
    const float t24_in = t.lodStep2Distance;
    const float t24_out = std::max(t12_out + 1.0f, t24_in - 8.0f);
    const float t48_in = t.lodStep4Distance;
    const float t48_out = std::max(t24_out + 1.0f, t48_in - 12.0f);
    const float t96_in = t.lodStep8Distance;
    const float t96_out = std::max(t48_out + 1.0f, t96_in - 18.0f);
    const float t192_in = t.lodStep16Distance;
    const float t192_out = std::max(t96_out + 1.0f, t192_in - 24.0f);

    int lodStep = st.lodStep;
    if (lodStep <= 1) {
      if (distHoriz > t12_in || viewDot < -0.10f) lodStep = 2;
    } else if (lodStep == 2) {
      if (distHoriz < t12_out && viewDot > 0.05f) lodStep = 1;
      else if (distHoriz > t24_in || viewDot < -0.15f) lodStep = 4;
    } else if (lodStep == 4) {
      if (distHoriz < t24_out && viewDot > 0.05f) lodStep = 2;
      else if (distHoriz > t48_in || viewDot < -0.20f) lodStep = 8;
    } else if (lodStep == 8) {
      if (distHoriz < t48_out && viewDot > 0.05f) lodStep = 4;
      else if (distHoriz > t192_in || viewDot < -0.25f) lodStep = 16;
    } else {
      if (distHoriz < t192_out && viewDot > 0.05f) lodStep = 8;
      else lodStep = 16;
    }

    // Lock near-camera detail so it doesn't pop right in front of you.
    if (distHoriz < t.lodForceNearDistance) lodStep = 1;
    else if (distHoriz < t.lodStep2Distance * 0.5f) lodStep = std::min(lodStep, 2);

    // If tess is off, bias toward coarser geo LOD (still stable via hysteresis above).
    if (!st.wantTess) lodStep = std::max(lodStep, 2);
    st.lodStep = lodStep;
  }
  recordPass("terrain_lod", passStart);

  // --- Shadow map pass (optional; off by default via RenderSettingsComponent) ---
  passStart = glfwGetTime();
  bool shadowOn = false;
  math::Mat4 lightViewProj{};
  math::Vec3 shadowLightDir{0.0f, -1.0f, 0.0f};
  float shadowBias = 0.001f;
  float shadowStrength = 1.0f;
  float shadowTexelX = 1.0f;
  float shadowTexelY = 1.0f;

  if (frame.settings.present && frame.settings.shadowsEnabled) {
    const ecs::systems::GraphicsSystem::LightDraw* sun = nullptr;
    for (const auto& l : frame.lights) {
      if (l.type == 0 && l.castShadows) {
        sun = &l;
        break;
      }
    }

    if (sun) {
      int q = frame.settings.shadowQuality;
      if (q < 0) q = 0;
      if (q > 2) q = 2;
      const float qScale = (q == 0) ? 0.75f : (q == 1 ? 1.0f : 1.5f);
      const int desiredRes =
          static_cast<int>(std::max(128.0f, std::min(4096.0f, static_cast<float>(sun->shadowResolution) * qScale)));
      ensureShadowMap(m_shadowFbo, m_shadowDepthTex, m_shadowRes, desiredRes);

      shadowTexelX = 1.0f / static_cast<float>(m_shadowRes);
      shadowTexelY = 1.0f / static_cast<float>(m_shadowRes);
      shadowStrength = frame.settings.shadowStrength;
      shadowBias = sun->shadowBias;
      shadowLightDir = math::normalize(sun->direction);

      const float dist = std::max(10.0f, sun->shadowDistance);

      // Focus the shadow frustum slightly ahead of the camera (better use of resolution).
      const math::Vec3 center = camPos + camFwd * (dist * 0.35f);
      const float radius = dist * 0.60f;

      const math::Vec3 up0{0.0f, 1.0f, 0.0f};
      const float upDot = std::abs(math::dot(up0, shadowLightDir));
      const math::Vec3 up = (upDot > 0.95f) ? math::Vec3{0.0f, 0.0f, 1.0f} : up0;

      const math::Vec3 eye = center - shadowLightDir * dist;
      math::Mat4 lView = math::lookAt(eye, center, up);

      // Stabilize the shadow map to the texel grid to reduce shimmering/flicker as the camera moves.
      const float texelWorld = (2.0f * radius) / static_cast<float>(std::max(1, m_shadowRes));
      const math::Vec3 centerLS = mulPoint(lView, center);
      const float snappedX = std::floor(centerLS.x / texelWorld + 0.5f) * texelWorld;
      const float snappedY = std::floor(centerLS.y / texelWorld + 0.5f) * texelWorld;
      const float dx = snappedX - centerLS.x;
      const float dy = snappedY - centerLS.y;
      lView = math::mul(math::translate(math::Vec3{-dx, -dy, 0.0f}), lView);
      const math::Mat4 lProj = ortho(-radius, radius, -radius, radius, 0.1f, dist * 2.2f);
      lightViewProj = math::mul(lProj, lView);

      // Render depth.
      glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
      glViewport(0, 0, m_shadowRes, m_shadowRes);
      glClear(GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      glDepthMask(GL_TRUE);
      glDisable(GL_BLEND);
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);

      // Terrain casters.
      for (const auto& t : frame.terrains) {
        if (!t.castShadows) continue;
        const float sizeX = static_cast<float>(std::max(2, t.gridWidth)) * t.cellSizeMeters;
        const float sizeZ = static_cast<float>(std::max(2, t.gridHeight)) * t.cellSizeMeters;
        const float halfX = 0.5f * sizeX;
        const float halfZ = 0.5f * sizeZ;
        const float nearestX = clampf(camPos.x, t.position.x - halfX, t.position.x + halfX);
        const float nearestZ = clampf(camPos.z, t.position.z - halfZ, t.position.z + halfZ);
        const float shadowDist = std::sqrt((nearestX - camPos.x) * (nearestX - camPos.x) +
                                           (nearestZ - camPos.z) * (nearestZ - camPos.z));
        if (shadowDist > t.lodMaxRenderDistance) continue;

        auto it = m_terrainLodState.find(static_cast<std::uint32_t>(t.entity));
        const TerrainLodState st = (it != m_terrainLodState.end()) ? it->second : TerrainLodState{};
        const bool wantTess = frame.settings.shadowUseTessellation && st.wantTess;
        const char* key = wantTess ? "graphics/shaders/terrain_shadow" : "graphics/shaders/terrain_shadow_notess";
        const ShaderService::Program* program = m_shaders.getOrCreate(key);
        if (!program || !program->programId) continue;

        int lodStep = std::max(1, st.lodStep);
        if (!wantTess) lodStep = std::max(lodStep, 2);

        TerrainMesh* mesh = getOrCreateTerrainMesh(t, lodStep);
        if (!mesh || !mesh->vao) continue;

        glUseProgram(program->programId);
        const GLint locModel = glGetUniformLocation(program->programId, "u_Model");
        if (locModel >= 0) {
          const math::Mat4 model = math::translate(t.position);
          glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);
        }
        const GLint locLvp = glGetUniformLocation(program->programId, "u_LightViewProj");
        if (locLvp >= 0) glUniformMatrix4fv(locLvp, 1, GL_FALSE, lightViewProj.m);

        if (program->hasTessellation) {
          glPatchParameteri(GL_PATCH_VERTICES, 3);
          const GLint locCam = glGetUniformLocation(program->programId, "u_CameraPos");
          if (locCam >= 0) glUniform3f(locCam, camPos.x, camPos.y, camPos.z);
          const GLint locCamFwd = glGetUniformLocation(program->programId, "u_CameraForward");
          if (locCamFwd >= 0) glUniform3f(locCamFwd, camFwd.x, camFwd.y, camFwd.z);
          const GLint locTessNear = glGetUniformLocation(program->programId, "u_TessNear");
          const GLint locTessFar = glGetUniformLocation(program->programId, "u_TessFar");
          const GLint locTessMin = glGetUniformLocation(program->programId, "u_TessMin");
          const GLint locTessMax = glGetUniformLocation(program->programId, "u_TessMax");
          if (locTessNear >= 0) glUniform1f(locTessNear, t.tessNear);
          if (locTessFar >= 0) glUniform1f(locTessFar, t.tessFar);
          if (locTessMin >= 0) glUniform1f(locTessMin, t.tessMin);
          if (locTessMax >= 0) glUniform1f(locTessMax, t.tessMax);
        }

        glBindVertexArray(mesh->vao);
        const GLenum mode = program->hasTessellation ? GL_PATCHES : GL_TRIANGLES;
        glDrawElements(mode, static_cast<GLsizei>(mesh->indexCount), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
	      }

	      // Mesh casters.
	      const ShaderService::Program* meshShadowProg = m_shaders.getOrCreate("graphics/shaders/model_shadow");
	      if (meshShadowProg && meshShadowProg->programId) {
	        glUseProgram(meshShadowProg->programId);
	        const GLint locLvp = glGetUniformLocation(meshShadowProg->programId, "u_LightViewProj");
	        if (locLvp >= 0) glUniformMatrix4fv(locLvp, 1, GL_FALSE, lightViewProj.m);
	        for (const auto& m : frame.meshes) {
	          if (!m.visible || !m.castShadows) continue;
	          GpuMeshAsset* gpu = getOrCreateGpuMesh(m.meshData.key);
	          if (!gpu || !gpu->ready) continue;
	          const GLint locModel = glGetUniformLocation(meshShadowProg->programId, "u_Model");
	          if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, m.modelMatrix.m);
	          for (const auto& sm : gpu->subMeshes) {
	            glBindVertexArray(sm.vao);
	            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sm.indexCount), GL_UNSIGNED_INT, nullptr);
	          }
	          glBindVertexArray(0);
	        }
	      }

	      // Rock casters (use existing instance chunks).
	      const ShaderService::Program* rockProg = m_shaders.getOrCreate("graphics/shaders/rocks_shadow");
      if (rockProg && rockProg->programId) {
        glUseProgram(rockProg->programId);
        const GLint locLvp = glGetUniformLocation(rockProg->programId, "u_LightViewProj");
        if (locLvp >= 0) glUniformMatrix4fv(locLvp, 1, GL_FALSE, lightViewProj.m);

        // Choose a ground terrain to anchor rock height (first terrain for now).
        const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain =
            frame.terrains.empty() ? nullptr : &frame.terrains[0];

  for (const auto& r : frame.rocks) {
          if (!r.castShadows) continue;
          RockMesh* mesh = getOrCreateRockMesh(r, groundTerrain);
          if (!mesh || !mesh->vao || mesh->instanceCapacity == 0 || mesh->chunks.empty()) continue;

          const GLint locOrigin = glGetUniformLocation(rockProg->programId, "u_InstanceOrigin");
          if (locOrigin >= 0) glUniform3f(locOrigin, r.position.x, r.position.y, r.position.z);

          glBindVertexArray(mesh->vao);
          glBindBuffer(GL_ARRAY_BUFFER, mesh->instanceVbo);

          const float lodBias = std::max(0.25f, r.lodBias);
          const float maxDist = 120.0f / lodBias;
          for (const auto& c : mesh->chunks) {
            const math::Vec3 chunkCenter = r.position + c.centerLocal;
            const math::Vec3 d0 = frame.camera.position - chunkCenter;
            const float distC = std::sqrt(d0.x * d0.x + d0.y * d0.y + d0.z * d0.z) - c.radius;
            if (distC > maxDist) continue;

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
      }

      glCullFace(GL_BACK);
      glBindFramebuffer(GL_FRAMEBUFFER, mainFbo);
      glViewport(0, 0, sceneW, sceneH);
      shadowOn = true;
    }
  }
  recordPass("shadow_pass", passStart);

  // Sky pass (fullscreen procedural).
  passStart = glfwGetTime();
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
  recordPass("sky_pass", passStart);

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

  // Main geometry passes: reset critical GL state (some earlier passes disable/modify these).
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_BLEND);
  glCullFace(GL_BACK);

  passStart = glfwGetTime();
  for (const auto& t : frame.terrains) {
    auto it = m_terrainLodState.find(static_cast<std::uint32_t>(t.entity));
    const TerrainLodState st = (it != m_terrainLodState.end()) ? it->second : TerrainLodState{};
    const float sizeX = static_cast<float>(std::max(2, t.gridWidth)) * t.cellSizeMeters;
    const float sizeZ = static_cast<float>(std::max(2, t.gridHeight)) * t.cellSizeMeters;
    const float halfX = 0.5f * sizeX;
    const float halfZ = 0.5f * sizeZ;
    const float nearestX = clampf(camPos.x, t.position.x - halfX, t.position.x + halfX);
    const float nearestZ = clampf(camPos.z, t.position.z - halfZ, t.position.z + halfZ);
    const float mainDist = std::sqrt((nearestX - camPos.x) * (nearestX - camPos.x) + (nearestZ - camPos.z) * (nearestZ - camPos.z));
    if (mainDist > t.lodMaxRenderDistance) continue;

    std::string shaderKey = t.shader.key;
    if (!st.wantTess) shaderKey += "_notess";

    const ShaderService::Program* program = m_shaders.getOrCreate(shaderKey);
    if (!program || !program->programId) continue;

    const ecs::systems::GraphicsSystem::FrameSnapshot::FogDraw* fog =
        frame.fogVolumes.empty() ? nullptr : &frame.fogVolumes[0];

    const int lodStep = std::max(1, st.lodStep);

    TerrainMesh* mesh = getOrCreateTerrainMesh(t, lodStep);
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
    const GLint locCamFwd = glGetUniformLocation(program->programId, "u_CameraForward");
    if (locCamFwd >= 0) glUniform3f(locCamFwd, frame.camera.forward.x, frame.camera.forward.y, frame.camera.forward.z);
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

    // Shadow uniforms (single directional shadow map, optional).
    const GLint locShadowOn = glGetUniformLocation(program->programId, "u_ShadowEnabled");
    if (locShadowOn >= 0) glUniform1i(locShadowOn, shadowOn ? 1 : 0);
    if (shadowOn) {
      const GLint locLvp = glGetUniformLocation(program->programId, "u_LightViewProj");
      if (locLvp >= 0) glUniformMatrix4fv(locLvp, 1, GL_FALSE, lightViewProj.m);
      const GLint locBias = glGetUniformLocation(program->programId, "u_ShadowBias");
      if (locBias >= 0) glUniform1f(locBias, shadowBias);
      const GLint locStrength = glGetUniformLocation(program->programId, "u_ShadowStrength");
      if (locStrength >= 0) glUniform1f(locStrength, shadowStrength);
      const GLint locTexel = glGetUniformLocation(program->programId, "u_ShadowTexelSize");
      if (locTexel >= 0) glUniform2f(locTexel, shadowTexelX, shadowTexelY);

      glActiveTexture(GL_TEXTURE15);
      glBindTexture(GL_TEXTURE_2D, m_shadowDepthTex);
      const GLint locMap = glGetUniformLocation(program->programId, "u_ShadowMap");
      if (locMap >= 0) glUniform1i(locMap, 15);
      glActiveTexture(GL_TEXTURE0);
    }

    // Far grass tint (disabled for now).
    const GLint locGtOn = glGetUniformLocation(program->programId, "u_GrassTintEnabled");
    if (locGtOn >= 0) glUniform1i(locGtOn, 0);

    const GLint locBase = glGetUniformLocation(program->programId, "u_BaseColor");
    if (locBase >= 0) glUniform4f(locBase, t.baseColorR, t.baseColorG, t.baseColorB, 1.0f);
    const GLint locRough = glGetUniformLocation(program->programId, "u_Roughness");
    if (locRough >= 0) glUniform1f(locRough, t.roughness);
    const GLint locRoughInv = glGetUniformLocation(program->programId, "u_RoughnessInvert");
    if (locRoughInv >= 0) glUniform1i(locRoughInv, t.roughnessInvert ? 1 : 0);
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
    const GLint locDispInv = glGetUniformLocation(program->programId, "u_DisplacementInvert");
    if (locDispInv >= 0) glUniform1i(locDispInv, t.displacementInvert ? 1 : 0);

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

      // Tess quality (renderer-defined): keep tessellation modest by default.
      // 0=Low, 1=Medium, 2=High.
      int q = t.tessQuality;
      if (q < 0) q = 0;
      if (q > 2) q = 2;
      const float qScale = (q == 0) ? 0.45f : (q == 1 ? 0.70f : 1.0f);
      const float qFarScale = (q == 0) ? 0.65f : (q == 1 ? 0.85f : 1.0f);

      const float tessNear = t.tessNear;
      const float tessFar = std::max(tessNear + 1.0f, t.tessFar * qFarScale);
      const float tessMin = std::max(1.0f, t.tessMin);
      const float tessMax = std::max(tessMin, std::min(64.0f, t.tessMax * qScale));

      const GLint locTessNear = glGetUniformLocation(program->programId, "u_TessNear");
      const GLint locTessFar = glGetUniformLocation(program->programId, "u_TessFar");
      const GLint locTessMin = glGetUniformLocation(program->programId, "u_TessMin");
      const GLint locTessMax = glGetUniformLocation(program->programId, "u_TessMax");
      if (locTessNear >= 0) glUniform1f(locTessNear, tessNear);
      if (locTessFar >= 0) glUniform1f(locTessFar, tessFar);
      if (locTessMin >= 0) glUniform1f(locTessMin, tessMin);
      if (locTessMax >= 0) glUniform1f(locTessMax, tessMax);

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
  recordPass("terrain_pass", passStart);

  // Choose a ground terrain to anchor procedural scatters (first terrain for now).
  const ecs::systems::GraphicsSystem::TerrainDraw* groundTerrain = frame.terrains.empty() ? nullptr : &frame.terrains[0];

  // Grass pass (GPU-instanced clumps; no ECS entity per blade).
  passStart = glfwGetTime();
  if (!frame.grasses.empty()) {
    struct GrassInstance {
      float px, py, pz;
      float scale;
      float rot;
      float var;
    };

    const float timeSeconds = static_cast<float>(glfwGetTime());

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

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    const auto findTerrain = [&](ecs::EntityId id) -> const ecs::systems::GraphicsSystem::TerrainDraw* {
      for (const auto& t : frame.terrains) {
        if (t.entity == id) return &t;
      }
      return nullptr;
    };

    for (const auto& gr : frame.grasses) {
      const ecs::systems::GraphicsSystem::TerrainDraw* gt =
          (gr.sourceTerrainEntity != ecs::kInvalidEntityId) ? findTerrain(gr.sourceTerrainEntity) : groundTerrain;

      const std::string shaderKey = gr.shader.key.empty() ? "graphics/shaders/grass" : gr.shader.key;
      const ShaderService::Program* program = m_shaders.getOrCreate(shaderKey);
      if (!program || !program->programId) continue;

      glUseProgram(program->programId);

      const GLint locVp = glGetUniformLocation(program->programId, "u_ViewProj");
      if (locVp >= 0) glUniformMatrix4fv(locVp, 1, GL_FALSE, viewProj.m);
      const GLint locCam = glGetUniformLocation(program->programId, "u_CameraPos");
      if (locCam >= 0) glUniform3f(locCam, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
      const GLint locTime = glGetUniformLocation(program->programId, "u_Time");
      if (locTime >= 0) glUniform1f(locTime, timeSeconds);

      const GLint locSunDir = glGetUniformLocation(program->programId, "u_SunDir");
      if (locSunDir >= 0) glUniform3f(locSunDir, sunDir.x, sunDir.y, sunDir.z);
      const GLint locSunCol = glGetUniformLocation(program->programId, "u_SunColor");
      if (locSunCol >= 0) glUniform3f(locSunCol, sunCol.x, sunCol.y, sunCol.z);
      const GLint locSunInt = glGetUniformLocation(program->programId, "u_SunIntensity");
      if (locSunInt >= 0) glUniform1f(locSunInt, sunIntensity);
      const GLint locOrigin = glGetUniformLocation(program->programId, "u_InstanceOrigin");
      if (locOrigin >= 0) glUniform3f(locOrigin, gr.position.x, gr.position.y, gr.position.z);

      int maxBoundTextureUnits = 0;

      const int interactionCount = gr.interactionEnabled ? 1 : 0;
      const GLint locIc = glGetUniformLocation(program->programId, "u_InteractionCount");
      if (locIc >= 0) glUniform1i(locIc, interactionCount);
      if (interactionCount > 0) {
        const GLint locI0 = glGetUniformLocation(program->programId, "u_InteractionsPosRad[0]");
        if (locI0 >= 0) {
          glUniform4f(locI0,
                      frame.camera.position.x,
                      frame.camera.position.y,
                      frame.camera.position.z,
                      std::max(0.01f, gr.interactionRadiusMeters));
        }
        const GLint locS0 = glGetUniformLocation(program->programId, "u_InteractionsStrength[0]");
        if (locS0 >= 0) glUniform1f(locS0, gr.interactionStrength);
      }

      for (std::size_t li = 0; li < gr.layers.size(); ++li) {
        const auto& layer = gr.layers[li];
        const bool planeMode = (layer.species == "BillboardGrassPlanes");

        const GLint locMode = glGetUniformLocation(program->programId, "u_GrassMode");
        if (locMode >= 0) glUniform1i(locMode, planeMode ? 1 : 0);

        if (planeMode) {
          const char* samplerNames[6] = {
              "u_GrassTex0",
              "u_GrassTex1",
              "u_GrassTex2",
              "u_GrassTex3",
              "u_GrassTex4",
              "u_GrassTex5",
          };

          struct BoundGrassTex final {
            GLuint id = 0;
            float aspect = 1.0f;
          };
          BoundGrassTex bound[6]{};
          int texCount = 0;
          std::string texturePaths[6];

          if (!gr.grassTextures.empty()) {
            texCount = std::min(6, static_cast<int>(gr.grassTextures.size()));
            for (int i = 0; i < texCount; ++i) {
              texturePaths[i] = gr.grassTextures[static_cast<std::size_t>(i)].key;
              bound[i].id = m_textures.requestTexture(texturePaths[i], true);
            }
          } else if (gr.hasAlbedoTex && !gr.albedoTex.key.empty()) {
            texCount = 1;
            texturePaths[0] = gr.albedoTex.key;
            bound[0].id = m_textures.requestTexture(texturePaths[0], true);
          } else {
            texCount = 1;
            texturePaths[0] = kDefaultGrassBillboardTexture;
            bound[0].id = m_textures.requestTexture(texturePaths[0], true);
          }

          for (int texIndex = 0; texIndex < texCount; ++texIndex) {
            glActiveTexture(GL_TEXTURE0 + texIndex);
            glBindTexture(GL_TEXTURE_2D, bound[texIndex].id);

            if (const auto info = m_textures.getInfo(texturePaths[texIndex]); info && info->ready && info->height > 0) {
              bound[texIndex].aspect = static_cast<float>(info->width) / static_cast<float>(info->height);
            }

            const GLint loc = glGetUniformLocation(program->programId, samplerNames[texIndex]);
            if (loc >= 0) glUniform1i(loc, texIndex);
          }

          const GLint locCount = glGetUniformLocation(program->programId, "u_GrassTexCount");
          if (locCount >= 0) glUniform1i(locCount, texCount);
          float aspects[6] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
          for (int i = 0; i < texCount; ++i) aspects[i] = bound[i].aspect;
          const GLint locAspect = glGetUniformLocation(program->programId, "u_GrassTexAspect[0]");
          if (locAspect >= 0) glUniform1fv(locAspect, 6, aspects);

          maxBoundTextureUnits = std::max(maxBoundTextureUnits, texCount);
        } else {
          const std::string albedoPath = gr.hasAlbedoTex ? gr.albedoTex.key : "assets/textures/grass/grass_color.jpg";
          const GLuint albedoId = m_textures.requestTexture(albedoPath, true);
          const bool useAlbedo = albedoId != 0;
          const GLint locUseAlb = glGetUniformLocation(program->programId, "u_UseAlbedo");
          if (locUseAlb >= 0) glUniform1i(locUseAlb, useAlbedo ? 1 : 0);
          const GLint locAlbScale = glGetUniformLocation(program->programId, "u_AlbedoUvScale");
          if (locAlbScale >= 0) glUniform1f(locAlbScale, gr.albedoUvScale);
          if (useAlbedo) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, albedoId);
            const GLint loc = glGetUniformLocation(program->programId, "u_AlbedoTex");
            if (loc >= 0) glUniform1i(loc, 0);
          }
          maxBoundTextureUnits = std::max(maxBoundTextureUnits, 1);
        }

        GrassMesh* mesh = getOrCreateGrassMesh(gr, li, gt);
        if (!mesh || !mesh->vao || mesh->instanceCapacity == 0 || mesh->chunks.empty()) continue;

        const float lodBias = std::max(0.25f, gr.lodBias);
        const float maxDist = std::max(2.0f, layer.maxDistance / lodBias);
        const float fadeRange = std::min(8.0f, std::max(2.0f, maxDist * 0.25f));
        const float fadeNear = std::max(0.0f, maxDist - fadeRange);

        const GLint locFadeNear = glGetUniformLocation(program->programId, "u_FadeNear");
        const GLint locFadeFar = glGetUniformLocation(program->programId, "u_FadeFar");
        if (locFadeNear >= 0) glUniform1f(locFadeNear, fadeNear);
        if (locFadeFar >= 0) glUniform1f(locFadeFar, maxDist);

        math::Vec3 tint{0.26f, 0.52f, 0.18f};
        float carpetHaze = 0.35f;
        float stylizedBands = 0.35f;
        float carpetThickness = 0.65f;
        if (planeMode || layer.species == "BillboardGrassPlanes") {
          tint = {0.93f, 1.00f, 0.92f};
          carpetHaze = 0.60f;
          stylizedBands = 0.22f;
          carpetThickness = 0.58f;
        } else if (layer.species == "ShaderGrassCarpet") {
          tint = {0.24f, 0.49f, 0.15f};
          carpetHaze = 0.72f;
          stylizedBands = 0.12f;
          carpetThickness = 0.72f;
        } else if (layer.species == "GroundCover") tint = {0.20f, 0.45f, 0.16f};
        else if (layer.species == "DryGrass") tint = {0.44f, 0.50f, 0.18f};
        else if (layer.species == "BroadLeafGrass") tint = {0.28f, 0.56f, 0.20f};
        else if (layer.species == "SmallFlower") tint = {0.34f, 0.56f, 0.22f};
        const GLint locTint = glGetUniformLocation(program->programId, "u_SpeciesTint");
        if (locTint >= 0) glUniform3f(locTint, tint.x, tint.y, tint.z);
        const GLint locHaze = glGetUniformLocation(program->programId, "u_CarpetHaze");
        if (locHaze >= 0) glUniform1f(locHaze, carpetHaze);
        const GLint locBands = glGetUniformLocation(program->programId, "u_StylizedBands");
        if (locBands >= 0) glUniform1f(locBands, stylizedBands);
        const GLint locThickness = glGetUniformLocation(program->programId, "u_CarpetThickness");
        if (locThickness >= 0) glUniform1f(locThickness, carpetThickness);

        if (planeMode) {
          const float lodTwoPlaneDist = std::max(4.0f, maxDist * 0.36f);
          const float lodOnePlaneDist = std::max(lodTwoPlaneDist + 3.0f, maxDist * 0.72f);
          const GLint locLodTwo = glGetUniformLocation(program->programId, "u_LodTwoPlaneDist");
          if (locLodTwo >= 0) glUniform1f(locLodTwo, lodTwoPlaneDist);
          const GLint locLodOne = glGetUniformLocation(program->programId, "u_LodOnePlaneDist");
          if (locLodOne >= 0) glUniform1f(locLodOne, lodOnePlaneDist);
          const GLint locBottomFade = glGetUniformLocation(program->programId, "u_BottomFade");
          if (locBottomFade >= 0) glUniform1f(locBottomFade, 0.14f);
        } else {
          const GLint locWind = glGetUniformLocation(program->programId, "u_WindStrength");
          if (locWind >= 0) glUniform1f(locWind, layer.windStrength);
          const GLint locBend = glGetUniformLocation(program->programId, "u_BladeBendStrength");
          if (locBend >= 0) glUniform1f(locBend, mesh->bendStrength);
          const GLint locCurve = glGetUniformLocation(program->programId, "u_BladeCurveStrength");
          if (locCurve >= 0) glUniform1f(locCurve, mesh->curveStrength);
          const GLint locTwist = glGetUniformLocation(program->programId, "u_BladeTwistStrength");
          if (locTwist >= 0) glUniform1f(locTwist, mesh->twistStrength);
          const GLint locWindDir = glGetUniformLocation(program->programId, "u_WindDirXZ");
          if (locWindDir >= 0) glUniform2f(locWindDir, 0.92f, 0.38f);
          const GLint locWindSpd = glGetUniformLocation(program->programId, "u_WindSpeed");
          if (locWindSpd >= 0) glUniform1f(locWindSpd, 1.25f);
        }

        glBindVertexArray(mesh->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->instanceVbo);

        const math::Vec3 camFwdXZ = math::normalize(math::Vec3{frame.camera.forward.x, 0.0f, frame.camera.forward.z});
        for (const auto& c : mesh->chunks) {
          const math::Vec3 chunkCenter = gr.position + c.centerLocal;
          const math::Vec3 d = frame.camera.position - chunkCenter;
          const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z) - c.radius;
          if (dist > maxDist) continue;
          const math::Vec3 toChunkXZ{chunkCenter.x - frame.camera.position.x, 0.0f, chunkCenter.z - frame.camera.position.z};
          const float horizDist = std::sqrt(toChunkXZ.x * toChunkXZ.x + toChunkXZ.z * toChunkXZ.z);
          if (horizDist > 7.0f && (camFwdXZ.x != 0.0f || camFwdXZ.z != 0.0f)) {
            const float invLen = 1.0f / std::max(0.001f, horizDist);
            const float viewDot = (toChunkXZ.x * invLen) * camFwdXZ.x + (toChunkXZ.z * invLen) * camFwdXZ.z;
            const float sideAllowance = std::min(0.45f, c.radius / std::max(1.0f, horizDist));
            if (viewDot < (-0.28f - sideAllowance)) continue;
          }

          const std::size_t baseByte = static_cast<std::size_t>(c.instanceOffset) * sizeof(GrassInstance);
          glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GrassInstance), reinterpret_cast<void*>(baseByte + 0));
          glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(GrassInstance),
                                reinterpret_cast<void*>(baseByte + sizeof(float) * 3));
          glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GrassInstance),
                                reinterpret_cast<void*>(baseByte + sizeof(float) * 4));
          glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(GrassInstance),
                                reinterpret_cast<void*>(baseByte + sizeof(float) * 5));
          glDrawElementsInstanced(GL_TRIANGLES,
                                  static_cast<GLsizei>(mesh->indexCount),
                                  GL_UNSIGNED_INT,
                                  nullptr,
                                  static_cast<GLsizei>(c.instanceCount));
        }

        glBindVertexArray(0);
      }

      for (int texIndex = 0; texIndex < maxBoundTextureUnits; ++texIndex) {
        glActiveTexture(GL_TEXTURE0 + texIndex);
        glBindTexture(GL_TEXTURE_2D, 0);
      }
      glActiveTexture(GL_TEXTURE0);
    }
  }
  recordPass("grass_pass", passStart);

  // Mesh pass (async-loaded glTF/extension-based assets).
  passStart = glfwGetTime();
  for (const auto& m : frame.meshes) {
    if (!m.visible) continue;
    const ShaderService::Program* program = m_shaders.getOrCreate(m.shader.key.empty() ? "graphics/shaders/model" : m.shader.key);
    if (!program || !program->programId) continue;

    GpuMeshAsset* gpu = getOrCreateGpuMesh(m.meshData.key);
    if (!gpu || !gpu->ready) continue;

    glUseProgram(program->programId);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const GLint locModel = glGetUniformLocation(program->programId, "u_Model");
    if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, m.modelMatrix.m);
    const GLint locViewProj = glGetUniformLocation(program->programId, "u_ViewProj");
    if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
    const GLint locCam = glGetUniformLocation(program->programId, "u_CameraPos");
    if (locCam >= 0) glUniform3f(locCam, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
    const GLint locSkinned = glGetUniformLocation(program->programId, "u_Skinned");
    if (locSkinned >= 0) glUniform1i(locSkinned, m.hasSkinning ? 1 : 0);
    if (m.hasSkinning && m.skinMatrixCount > 0) {
      const GLint locBones = glGetUniformLocation(program->programId, "u_Bones[0]");
      if (locBones >= 0) {
        glUniformMatrix4fv(locBones,
                           static_cast<GLsizei>(std::min<std::size_t>(96, m.skinMatrixCount)),
                           GL_FALSE,
                           m.skinMatrices[0].m);
      }
    }

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
    const GLint locSunInt = glGetUniformLocation(program->programId, "u_SunIntensity");
    if (locSunInt >= 0) glUniform1f(locSunInt, sunIntensity);

    const GLint locShadowOn = glGetUniformLocation(program->programId, "u_ShadowEnabled");
    if (locShadowOn >= 0) glUniform1i(locShadowOn, (shadowOn && m.receiveShadows) ? 1 : 0);
    if (shadowOn && m.receiveShadows) {
      const GLint locLvp = glGetUniformLocation(program->programId, "u_LightViewProj");
      if (locLvp >= 0) glUniformMatrix4fv(locLvp, 1, GL_FALSE, lightViewProj.m);
      const GLint locBias = glGetUniformLocation(program->programId, "u_ShadowBias");
      if (locBias >= 0) glUniform1f(locBias, shadowBias);
      const GLint locStrength = glGetUniformLocation(program->programId, "u_ShadowStrength");
      if (locStrength >= 0) glUniform1f(locStrength, shadowStrength);
      const GLint locTexel = glGetUniformLocation(program->programId, "u_ShadowTexelSize");
      if (locTexel >= 0) glUniform2f(locTexel, shadowTexelX, shadowTexelY);
      glActiveTexture(GL_TEXTURE15);
      glBindTexture(GL_TEXTURE_2D, m_shadowDepthTex);
      const GLint locMap = glGetUniformLocation(program->programId, "u_ShadowMap");
      if (locMap >= 0) glUniform1i(locMap, 15);
      glActiveTexture(GL_TEXTURE0);
    }

    if (program->hasTessellation) {
      glPatchParameteri(GL_PATCH_VERTICES, 3);
      int q = m.tessQuality;
      if (q < 0) q = 0;
      if (q > 2) q = 2;
      const float qScale = (q == 0) ? 0.45f : (q == 1 ? 0.70f : 1.0f);
      const float qFarScale = (q == 0) ? 0.65f : (q == 1 ? 0.85f : 1.0f);

      const float tessNear = m.tessNear;
      const float tessFar = std::max(tessNear + 1.0f, m.tessFar * qFarScale);
      const float tessMin = std::max(1.0f, m.tessMin);
      const float tessMax = std::max(tessMin, std::min(64.0f, m.tessMax * qScale));

      const GLint locCam = glGetUniformLocation(program->programId, "u_CameraPos");
      if (locCam >= 0) glUniform3f(locCam, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
      const GLint locCamFwd = glGetUniformLocation(program->programId, "u_CameraForward");
      if (locCamFwd >= 0) glUniform3f(locCamFwd, frame.camera.forward.x, frame.camera.forward.y, frame.camera.forward.z);
      const GLint locTessNear = glGetUniformLocation(program->programId, "u_TessNear");
      const GLint locTessFar = glGetUniformLocation(program->programId, "u_TessFar");
      const GLint locTessMin = glGetUniformLocation(program->programId, "u_TessMin");
      const GLint locTessMax = glGetUniformLocation(program->programId, "u_TessMax");
      if (locTessNear >= 0) glUniform1f(locTessNear, tessNear);
      if (locTessFar >= 0) glUniform1f(locTessFar, tessFar);
      if (locTessMin >= 0) glUniform1f(locTessMin, tessMin);
      if (locTessMax >= 0) glUniform1f(locTessMax, tessMax);
    }

    for (const auto& sm : gpu->subMeshes) {
      const assets::MeshMaterial* mat =
          sm.materialIndex < gpu->data.materials.size() ? &gpu->data.materials[sm.materialIndex] : nullptr;
      const math::Vec3 matBase = mat ? mat->baseColorFactor : math::Vec3{1.0f, 1.0f, 1.0f};
      const math::Vec3 finalBaseColor{
          (m.hasBaseColorParam ? m.baseColorR : 1.0f) * matBase.x,
          (m.hasBaseColorParam ? m.baseColorG : 1.0f) * matBase.y,
          (m.hasBaseColorParam ? m.baseColorB : 1.0f) * matBase.z,
      };
      const float finalRoughness = m.hasRoughnessParam ? m.roughness : (mat ? mat->roughnessFactor : m.roughness);
      const float finalMetallic = m.hasMetallicParam ? m.metallic : (mat ? mat->metallicFactor : m.metallic);
      const float finalSpecular = m.hasSpecularIntensityParam ? m.specularIntensity : (mat ? mat->specularFactor : m.specularIntensity);
      const float finalNormalStrength = m.hasNormalStrengthParam ? m.normalStrength : (mat ? mat->normalScale : m.normalStrength);
      const float finalAoStrength = m.hasAoStrengthParam ? m.aoStrength : (mat ? mat->occlusionStrength : m.aoStrength);
      const math::Vec3 finalEmissiveColor = m.hasEmissiveColorParam
                                                ? math::Vec3{m.emissiveColorR, m.emissiveColorG, m.emissiveColorB}
                                                : (mat ? mat->emissiveFactor : math::Vec3{0.0f, 0.0f, 0.0f});
      const float finalEmissiveStrength = m.hasEmissiveStrengthParam
                                              ? m.emissiveStrength
                                              : ((mat && (mat->emissiveFactor.x > 0.0f || mat->emissiveFactor.y > 0.0f ||
                                                          mat->emissiveFactor.z > 0.0f))
                                                     ? 1.0f
                                                     : m.emissiveStrength);
      const float finalDisplacementStrength =
          m.hasDisplacementStrengthParam ? m.displacementStrength : m.displacementStrength;
      const GLuint albedo = m.hasAlbedoTex ? m_textures.requestTexture(m.albedoTex.key, true)
                                           : ((mat && !mat->baseColorTexture.empty())
                                                  ? m_textures.requestTexture(mat->baseColorTexture, true)
                                                  : 0);
      const GLuint normal = m.hasNormalTex ? m_textures.requestTexture(m.normalTex.key, false)
                                           : ((mat && !mat->normalTexture.empty())
                                                  ? m_textures.requestTexture(mat->normalTexture, false)
                                                  : 0);
      const GLuint metallicRoughness =
          m.hasMetallicRoughnessTex ? m_textures.requestTexture(m.metallicRoughnessTex.key, false)
                                    : ((mat && !mat->metallicRoughnessTexture.empty())
                                           ? m_textures.requestTexture(mat->metallicRoughnessTexture, false)
                                           : 0);
      const GLuint roughness = m.hasRoughnessTex ? m_textures.requestTexture(m.roughnessTex.key, false) : 0;
      const GLuint metallic = m.hasMetallicTex ? m_textures.requestTexture(m.metallicTex.key, false) : 0;
      const GLuint ao = m.hasAoTex ? m_textures.requestTexture(m.aoTex.key, false)
                                   : ((mat && !mat->occlusionTexture.empty())
                                          ? m_textures.requestTexture(mat->occlusionTexture, false)
                                          : 0);
      const GLuint specular = m.hasSpecularTex ? m_textures.requestTexture(m.specularTex.key, false)
                                               : ((mat && !mat->specularTexture.empty())
                                                      ? m_textures.requestTexture(mat->specularTexture, false)
                                                      : 0);
      const GLuint emissive = m.hasEmissiveTex ? m_textures.requestTexture(m.emissiveTex.key, true)
                                               : ((mat && !mat->emissiveTexture.empty())
                                                      ? m_textures.requestTexture(mat->emissiveTexture, true)
                                                      : 0);
      const GLuint displacement = m.hasDisplacementTex ? m_textures.requestTexture(m.displacementTex.key, false) : 0;
      const GLuint orm = m.hasOrmTex ? m_textures.requestTexture(m.ormTex.key, false) : 0;
      const GLint locBase = glGetUniformLocation(program->programId, "u_BaseColor");
      if (locBase >= 0) {
        glUniform3f(locBase, finalBaseColor.x, finalBaseColor.y, finalBaseColor.z);
      }
      const GLint locRoughness = glGetUniformLocation(program->programId, "u_Roughness");
      if (locRoughness >= 0) glUniform1f(locRoughness, finalRoughness);
      const GLint locMetallic = glGetUniformLocation(program->programId, "u_Metallic");
      if (locMetallic >= 0) glUniform1f(locMetallic, finalMetallic);
      const GLint locSpecIntensity = glGetUniformLocation(program->programId, "u_SpecularIntensity");
      if (locSpecIntensity >= 0) glUniform1f(locSpecIntensity, finalSpecular);
      const GLint locNormalStrength = glGetUniformLocation(program->programId, "u_NormalStrength");
      if (locNormalStrength >= 0) glUniform1f(locNormalStrength, finalNormalStrength);
      const GLint locAoStrength = glGetUniformLocation(program->programId, "u_AOStrength");
      if (locAoStrength >= 0) glUniform1f(locAoStrength, finalAoStrength);
      const GLint locEmissiveColor = glGetUniformLocation(program->programId, "u_EmissiveColor");
      if (locEmissiveColor >= 0) glUniform3f(locEmissiveColor, finalEmissiveColor.x, finalEmissiveColor.y, finalEmissiveColor.z);
      const GLint locEmissiveStrength = glGetUniformLocation(program->programId, "u_EmissiveStrength");
      if (locEmissiveStrength >= 0) glUniform1f(locEmissiveStrength, finalEmissiveStrength);
      const GLint locDisplacementStrength = glGetUniformLocation(program->programId, "u_DisplacementStrength");
      if (locDisplacementStrength >= 0) glUniform1f(locDisplacementStrength, finalDisplacementStrength);
      const GLint locUseAlbedo = glGetUniformLocation(program->programId, "u_UseAlbedo");
      if (locUseAlbedo >= 0) glUniform1i(locUseAlbedo, albedo != 0 ? 1 : 0);
      if (albedo != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo);
        const GLint loc = glGetUniformLocation(program->programId, "u_AlbedoTex");
        if (loc >= 0) glUniform1i(loc, 0);
      }
      const GLint locUseNormal = glGetUniformLocation(program->programId, "u_UseNormal");
      if (locUseNormal >= 0) glUniform1i(locUseNormal, normal != 0 ? 1 : 0);
      if (normal != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal);
        const GLint loc = glGetUniformLocation(program->programId, "u_NormalTex");
        if (loc >= 0) glUniform1i(loc, 1);
      }
      const GLint locUseRoughness = glGetUniformLocation(program->programId, "u_UseRoughness");
      if (locUseRoughness >= 0) glUniform1i(locUseRoughness, roughness != 0 ? 1 : 0);
      if (roughness != 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, roughness);
        const GLint loc = glGetUniformLocation(program->programId, "u_RoughnessTex");
        if (loc >= 0) glUniform1i(loc, 2);
      }
      const GLint locUseMetallic = glGetUniformLocation(program->programId, "u_UseMetallic");
      if (locUseMetallic >= 0) glUniform1i(locUseMetallic, metallic != 0 ? 1 : 0);
      if (metallic != 0) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, metallic);
        const GLint loc = glGetUniformLocation(program->programId, "u_MetallicTex");
        if (loc >= 0) glUniform1i(loc, 3);
      }
      const GLint locUseAo = glGetUniformLocation(program->programId, "u_UseAO");
      if (locUseAo >= 0) glUniform1i(locUseAo, ao != 0 ? 1 : 0);
      if (ao != 0) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ao);
        const GLint loc = glGetUniformLocation(program->programId, "u_AOTex");
        if (loc >= 0) glUniform1i(loc, 4);
      }
      const GLint locUseSpecular = glGetUniformLocation(program->programId, "u_UseSpecular");
      if (locUseSpecular >= 0) glUniform1i(locUseSpecular, specular != 0 ? 1 : 0);
      if (specular != 0) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, specular);
        const GLint loc = glGetUniformLocation(program->programId, "u_SpecularTex");
        if (loc >= 0) glUniform1i(loc, 5);
      }
      const GLint locUseEmissive = glGetUniformLocation(program->programId, "u_UseEmissive");
      if (locUseEmissive >= 0) glUniform1i(locUseEmissive, emissive != 0 ? 1 : 0);
      if (emissive != 0) {
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, emissive);
        const GLint loc = glGetUniformLocation(program->programId, "u_EmissiveTex");
        if (loc >= 0) glUniform1i(loc, 6);
      }
      const GLint locUseDisplacement = glGetUniformLocation(program->programId, "u_UseDisplacement");
      if (locUseDisplacement >= 0) glUniform1i(locUseDisplacement, displacement != 0 ? 1 : 0);
      if (displacement != 0) {
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, displacement);
        const GLint loc = glGetUniformLocation(program->programId, "u_DisplacementTex");
        if (loc >= 0) glUniform1i(loc, 7);
      }
      const GLint locUseMetallicRoughness = glGetUniformLocation(program->programId, "u_UseMetallicRoughness");
      if (locUseMetallicRoughness >= 0) glUniform1i(locUseMetallicRoughness, metallicRoughness != 0 ? 1 : 0);
      if (metallicRoughness != 0) {
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, metallicRoughness);
        const GLint loc = glGetUniformLocation(program->programId, "u_MetallicRoughnessTex");
        if (loc >= 0) glUniform1i(loc, 8);
      }
      const GLint locUseOrm = glGetUniformLocation(program->programId, "u_UseOrm");
      if (locUseOrm >= 0) glUniform1i(locUseOrm, orm != 0 ? 1 : 0);
      if (orm != 0) {
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, orm);
        const GLint loc = glGetUniformLocation(program->programId, "u_OrmTex");
        if (loc >= 0) glUniform1i(loc, 9);
      }

      glBindVertexArray(sm.vao);
      const GLenum mode = program->hasTessellation ? GL_PATCHES : GL_TRIANGLES;
      glDrawElements(mode, static_cast<GLsizei>(sm.indexCount), GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
    for (int texUnit = 0; texUnit <= 9; ++texUnit) {
      glActiveTexture(GL_TEXTURE0 + texUnit);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
  }
  recordPass("mesh_pass", passStart);

  // Rocks pass (true 3D instances).
  passStart = glfwGetTime();
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

    // Shadow uniforms (optional).
    const GLint locShadowOn = glGetUniformLocation(program->programId, "u_ShadowEnabled");
    if (locShadowOn >= 0) glUniform1i(locShadowOn, shadowOn ? 1 : 0);
    if (shadowOn) {
      const GLint locLvp = glGetUniformLocation(program->programId, "u_LightViewProj");
      if (locLvp >= 0) glUniformMatrix4fv(locLvp, 1, GL_FALSE, lightViewProj.m);
      const GLint locBias = glGetUniformLocation(program->programId, "u_ShadowBias");
      if (locBias >= 0) glUniform1f(locBias, shadowBias);
      const GLint locStrength = glGetUniformLocation(program->programId, "u_ShadowStrength");
      if (locStrength >= 0) glUniform1f(locStrength, shadowStrength);
      const GLint locTexel = glGetUniformLocation(program->programId, "u_ShadowTexelSize");
      if (locTexel >= 0) glUniform2f(locTexel, shadowTexelX, shadowTexelY);

      glActiveTexture(GL_TEXTURE15);
      glBindTexture(GL_TEXTURE_2D, m_shadowDepthTex);
      const GLint locMap = glGetUniformLocation(program->programId, "u_ShadowMap");
      if (locMap >= 0) glUniform1i(locMap, 15);
      glActiveTexture(GL_TEXTURE0);
    }

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->instanceVbo);

    const float lodBias = std::max(0.25f, r.lodBias);
    const float maxDist = 120.0f / lodBias;
    for (const auto& c : mesh->chunks) {
      const math::Vec3 chunkCenter = r.position + c.centerLocal;
      const math::Vec3 d = frame.camera.position - chunkCenter;
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
  recordPass("rock_pass", passStart);

  // VFX pass.
  passStart = glfwGetTime();
  if (!frame.vfx.empty() && m_vfxProgram && m_vfxVao && m_vfxVbo) {
    std::vector<VfxVert> verts;
    verts.reserve(frame.vfx.size() * 96);

    const float timeSeconds = static_cast<float>(glfwGetTime());
    const math::Vec3 camFwd = math::normalize(frame.camera.forward);
    math::Vec3 camRight = math::normalize(math::cross(camFwd, {0.0f, 1.0f, 0.0f}));
    if (math::lengthSq(camRight) < 1e-6f) camRight = {1.0f, 0.0f, 0.0f};

    auto pushPoint = [&](const math::Vec3& pos, const render::Color& color, float sizeMeters, float kind, float seed) {
      verts.push_back({pos.x, pos.y, pos.z, color.r, color.g, color.b, color.a, sizeMeters, kind, seed});
    };

    for (const auto& vfx : frame.vfx) {
      if (!vfx.enabled) continue;

      const math::Vec3 emitterRight = vfxBasisRight(vfx.rotation);
      const math::Vec3 emitterUp = vfxBasisUp(vfx.rotation);
      const math::Vec3 emitterForward = vfxBasisForward(vfx.rotation);
      const float scaleMeters = std::max(0.05f, std::max({std::abs(vfx.scale.x), std::abs(vfx.scale.y), std::abs(vfx.scale.z)}));
      const math::Vec3 emitterPos = vfx.position + emitterUp * (0.05f * scaleMeters);
      const math::Vec3 toCamera = frame.camera.position - emitterPos;
      const float dist = math::length(toCamera);
      if (dist > std::max(1.0f, vfx.maxRenderDistance)) continue;
      if (dist > vfx.lodFarDistance) {
        const math::Vec3 dirToCamera = math::normalize(toCamera);
        const float viewDot = math::dot(dirToCamera, camFwd);
        if (viewDot < vfx.viewDotBias) continue;
      }

      const int lodLevel = vfxLODLevel(vfx, dist);
      const ecs::VfxComponent::Quality effectiveQuality = vfx.autoQuality
                                                               ? (lodLevel >= 3 ? ecs::VfxComponent::Quality::Ultra
                                                                                : lodLevel == 2 ? ecs::VfxComponent::Quality::High
                                                                                                : lodLevel == 1 ? ecs::VfxComponent::Quality::Medium
                                                                                                                : ecs::VfxComponent::Quality::Low)
                                                               : vfx.quality;
      const float qualityScale = vfxQualityScale(effectiveQuality);
      const float lodScale = (dist < vfx.lodNearDistance) ? 1.35f : (dist < vfx.lodMidDistance) ? 1.0f
                                  : (dist < vfx.lodFarDistance)                                     ? 0.70f
                                                                                                 : 0.45f;
      const float budgetScale = std::max(0.2f, qualityScale * lodScale);
      const float intensityScale = std::max(0.0f, vfx.intensity);

      switch (vfx.type) {
        case ecs::VfxComponent::Type::Fire: {
          const int particleCount = std::max(8, static_cast<int>(std::round((18.0f + vfx.spawnRate * 2.0f) * budgetScale * intensityScale)));
          for (int i = 0; i < particleCount; ++i) {
            const std::uint32_t base = hashU32(vfx.seed + static_cast<std::uint32_t>(i) * 1664525u);
            const float phase = fract01(timeSeconds * std::max(0.1f, vfx.spawnRate) + hash01(base));
            const float flicker = 0.85f + 0.15f * std::sin(timeSeconds * vfx.flickerSpeed + hash01(base + 3u) * 12.0f);
            const float radius = vfx.spreadRadiusMeters * scaleMeters * (0.25f + 0.75f * hash01(base + 5u));
            const float height = vfx.heightMeters * scaleMeters;
            const float up = phase * height;
            const float wobble = (hash01(base + 7u) - 0.5f) * 2.0f * vfx.heatHazeStrength * scaleMeters;
            const math::Vec3 pos = emitterPos + emitterUp * up + emitterRight * (std::sin(timeSeconds * 3.0f + phase * 7.0f + hash01(base + 1u) * 6.0f) * radius) +
                                   emitterForward * (std::cos(timeSeconds * 2.7f + phase * 5.0f + hash01(base + 2u) * 6.0f) * radius) +
                                   emitterUp * wobble;
            const float mixT = std::clamp(phase * 1.15f, 0.0f, 1.0f);
            const render::Color color{
                vfx.primaryColor.r * (1.0f - mixT) + vfx.secondaryColor.r * mixT,
                vfx.primaryColor.g * (1.0f - mixT) + vfx.secondaryColor.g * mixT,
                vfx.primaryColor.b * (1.0f - mixT) + vfx.secondaryColor.b * mixT,
                vfx.primaryColor.a * flicker};
            const float sizeMeters = std::max(0.02f, vfx.sizeMeters * scaleMeters * (0.35f + 0.65f * (1.0f - phase)) *
                                                           (0.70f + 0.30f * hash01(base + 4u)));
            pushPoint(pos, color, sizeMeters, 0.0f, static_cast<float>(base));
          }
          break;
        }
        case ecs::VfxComponent::Type::Electricity: {
          const int branches = std::max(1, static_cast<int>(std::round(vfx.branchCount * budgetScale * intensityScale)));
          const int segments = std::max(3, static_cast<int>(std::round(vfx.segmentCount * std::max(0.65f, budgetScale))));
          for (int b = 0; b < branches; ++b) {
            const std::uint32_t base = hashU32(vfx.seed + static_cast<std::uint32_t>(b) * 374761393u);
            const float branchPhase = hash01(base + 1u);
            const math::Vec3 branchEnd =
                emitterPos + emitterForward * (vfx.chargeLengthMeters * scaleMeters) +
                emitterRight * ((branchPhase - 0.5f) * 2.0f * vfx.arcJitter * scaleMeters) +
                emitterUp * ((hash01(base + 2u) - 0.5f) * 2.0f * vfx.arcJitter * 0.7f * scaleMeters);
            for (int s = 0; s <= segments; ++s) {
              const float t = static_cast<float>(s) / static_cast<float>(segments);
              const math::Vec3 basePos = emitterPos * (1.0f - t) + branchEnd * t;
              const float waveA = std::sin(timeSeconds * vfx.pulseSpeed + t * 11.0f + branchPhase * 8.0f);
              const float waveB = std::cos(timeSeconds * (vfx.pulseSpeed * 0.71f) + t * 17.0f + hash01(base + 3u) * 9.0f);
              const float jitter = (hash01(base + static_cast<std::uint32_t>(s) + 9u) - 0.5f) * 2.0f;
              const math::Vec3 pos = basePos + emitterRight * (waveA * vfx.arcJitter * scaleMeters * 0.55f + jitter * 0.04f * scaleMeters) +
                                     emitterUp * (waveB * vfx.arcJitter * scaleMeters * 0.35f);
              const float lineSize = std::max(0.03f, vfx.sizeMeters * scaleMeters * 0.34f * (1.0f - 0.55f * t));
              const float alpha = (0.70f + 0.30f * std::sin(timeSeconds * 20.0f + t * 4.0f + branchPhase * 10.0f));
              const render::Color color{
                  vfx.primaryColor.r * 0.45f + vfx.secondaryColor.r * 0.55f,
                  vfx.primaryColor.g * 0.55f + vfx.secondaryColor.g * 0.45f,
                  vfx.primaryColor.b * 0.85f + vfx.secondaryColor.b * 0.15f,
                  vfx.primaryColor.a * alpha};
              pushPoint(pos, color, lineSize, 1.0f, static_cast<float>(base ^ static_cast<std::uint32_t>(s)));
            }
          }
          break;
        }
        case ecs::VfxComponent::Type::Sparks: {
          const int sparkCount = std::max(4, static_cast<int>(std::round(vfx.sparkCount * budgetScale * intensityScale)));
          for (int i = 0; i < sparkCount; ++i) {
            const std::uint32_t base = hashU32(vfx.seed + static_cast<std::uint32_t>(i) * 2246822519u);
            const float phase = fract01(timeSeconds * std::max(0.1f, vfx.spawnRate) + hash01(base));
            const float spread = vfx.sparkSpreadDegrees * 3.14159265358979323846f / 180.0f;
            const float yaw = (hash01(base + 1u) * 2.0f - 1.0f) * spread;
            const float pitch = (hash01(base + 2u) * 2.0f - 1.0f) * spread * 0.55f;
            const math::Vec3 dir = math::normalize(
                emitterForward * std::cos(pitch) * std::cos(yaw) + emitterRight * std::sin(yaw) + emitterUp * std::sin(pitch) +
                emitterUp * vfx.upwardBias);
            const float velocity = vfx.speedMetersPerSecond * scaleMeters * (0.45f + hash01(base + 3u)) * std::max(0.35f, budgetScale);
            const float arc = phase * vfx.sparkTrailLengthMeters * scaleMeters;
            const math::Vec3 pos = emitterPos + dir * (velocity * phase * 0.7f) + emitterUp * (arc * 0.45f) -
                                   emitterUp * (vfx.gravityScale * 0.6f * phase * phase * scaleMeters);
            const float fade = std::max(0.0f, 1.0f - phase / std::max(0.01f, vfx.sparkFadeSeconds));
            const render::Color color{
                vfx.secondaryColor.r,
                vfx.secondaryColor.g * 0.95f,
                vfx.primaryColor.b + 0.15f,
                vfx.primaryColor.a * fade};
            pushPoint(pos, color, vfx.sizeMeters * scaleMeters * (0.20f + 0.30f * fade), 2.0f, static_cast<float>(base));
          }
          break;
        }
        case ecs::VfxComponent::Type::Smoke: {
          const int particleCount = std::max(6, static_cast<int>(std::round((12.0f + vfx.spawnRate) * budgetScale * intensityScale)));
          for (int i = 0; i < particleCount; ++i) {
            const std::uint32_t base = hashU32(vfx.seed + static_cast<std::uint32_t>(i) * 747796405u);
            const float phase = fract01(timeSeconds * std::max(0.08f, vfx.spawnRate * 0.25f) + hash01(base));
            const float radius = vfx.spreadRadiusMeters * scaleMeters * (0.5f + hash01(base + 1u));
            const math::Vec3 pos = emitterPos + emitterUp * (phase * vfx.heightMeters * scaleMeters * 0.9f) +
                                   emitterRight * ((hash01(base + 2u) - 0.5f) * radius) +
                                   emitterForward * ((hash01(base + 3u) - 0.5f) * radius);
            const float grey = 0.28f + 0.18f * hash01(base + 4u);
            const render::Color color{grey, grey * 1.02f, grey * 1.06f, 0.55f * (1.0f - phase)};
            pushPoint(pos, color, vfx.sizeMeters * scaleMeters * (0.8f + 1.2f * phase), 3.0f, static_cast<float>(base));
          }
          break;
        }
        case ecs::VfxComponent::Type::Steam: {
          const int particleCount = std::max(6, static_cast<int>(std::round((10.0f + vfx.spawnRate) * budgetScale * intensityScale)));
          for (int i = 0; i < particleCount; ++i) {
            const std::uint32_t base = hashU32(vfx.seed + static_cast<std::uint32_t>(i) * 3266489917u);
            const float phase = fract01(timeSeconds * std::max(0.08f, vfx.spawnRate * 0.35f) + hash01(base));
            const float radius = vfx.spreadRadiusMeters * scaleMeters * (0.3f + 0.7f * hash01(base + 1u));
            const math::Vec3 pos = emitterPos + emitterUp * (phase * vfx.heightMeters * scaleMeters * 1.05f) +
                                   emitterRight * ((hash01(base + 2u) - 0.5f) * radius) +
                                   emitterForward * ((hash01(base + 3u) - 0.5f) * radius);
            const render::Color color{0.82f, 0.88f, 0.95f, 0.38f * (1.0f - phase)};
            pushPoint(pos, color, vfx.sizeMeters * scaleMeters * (1.0f + 1.25f * phase), 4.0f, static_cast<float>(base));
          }
          break;
        }
      }
    }

    if (!verts.empty()) {
      glUseProgram(m_vfxProgram);
      const GLint locViewProj = glGetUniformLocation(m_vfxProgram, "u_ViewProj");
      if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
      const GLint locTime = glGetUniformLocation(m_vfxProgram, "u_Time");
      if (locTime >= 0) glUniform1f(locTime, timeSeconds);
      const float pointScale = static_cast<float>(std::max(1, sceneH)) * 0.55f;
      const GLint locPointScale = glGetUniformLocation(m_vfxProgram, "u_PointScale");
      if (locPointScale >= 0) glUniform1f(locPointScale, pointScale);

      glEnable(GL_PROGRAM_POINT_SIZE);
      glDisable(GL_CULL_FACE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glEnable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);

      glBindVertexArray(m_vfxVao);
      glBindBuffer(GL_ARRAY_BUFFER, m_vfxVbo);
      if (verts.size() > m_vfxCapacityVerts) {
        m_vfxCapacityVerts = std::max<std::size_t>(verts.size(), m_vfxCapacityVerts * 2 + 128);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vfxCapacityVerts * sizeof(VfxVert)), nullptr, GL_DYNAMIC_DRAW);
      }
      glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(VfxVert)), verts.data());
      glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(verts.size()));
      glBindVertexArray(0);
      glDepthMask(GL_TRUE);
      glDisable(GL_BLEND);
      glDisable(GL_PROGRAM_POINT_SIZE);
      glEnable(GL_CULL_FACE);
    }
  }
  recordPass("vfx_pass", passStart);

  // HUD pass.
  passStart = glfwGetTime();
  if ((!frame.billboards.empty() || !frame.draw2d.empty()) && m_hudProgram && m_hudVao && m_hudVbo) {
    const math::Vec3 camFwd = math::normalize(frame.camera.forward);
    math::Vec3 camRight = math::normalize(math::cross(camFwd, {0.0f, 1.0f, 0.0f}));
    if (math::lengthSq(camRight) < 1e-6f) camRight = {1.0f, 0.0f, 0.0f};
    const math::Vec3 camUp = math::normalize(math::cross(camRight, camFwd));
    const double textureTimeSeconds = glfwGetTime();

    const auto pixelToNdc = [&](float px, float py) -> math::Vec3 {
      return {px / static_cast<float>(m_fbWidth) * 2.0f - 1.0f, 1.0f - py / static_cast<float>(m_fbHeight) * 2.0f, 0.0f};
    };

    const auto appendQuad = [](std::vector<HudVert>& out,
                               const math::Vec3& p0,
                               const math::Vec3& p1,
                               const math::Vec3& p2,
                               const math::Vec3& p3,
                               const render::Color& c0,
                               const render::Color& c1,
                               const render::Color& c2,
                               const render::Color& c3,
                               float mode,
                               float u0,
                               float v0,
                               float u1,
                               float v1) {
      // Note: uv0=(u0,v0) is bottom-left and uv1=(u1,v1) is top-right. We map top vertices to v1.
      out.push_back({p0.x, p0.y, p0.z, u0, v1, c0.r, c0.g, c0.b, c0.a, mode});
      out.push_back({p1.x, p1.y, p1.z, u1, v1, c1.r, c1.g, c1.b, c1.a, mode});
      out.push_back({p2.x, p2.y, p2.z, u1, v0, c2.r, c2.g, c2.b, c2.a, mode});
      out.push_back({p0.x, p0.y, p0.z, u0, v1, c0.r, c0.g, c0.b, c0.a, mode});
      out.push_back({p2.x, p2.y, p2.z, u1, v0, c2.r, c2.g, c2.b, c2.a, mode});
      out.push_back({p3.x, p3.y, p3.z, u0, v0, c3.r, c3.g, c3.b, c3.a, mode});
    };

    const auto axesFromRotation = [](const math::Vec3& rotationDeg, math::Vec3& outRight, math::Vec3& outUp) {
      const math::Mat4 rotationMatrix = composeTransform({0.0f, 0.0f, 0.0f}, rotationDeg, {1.0f, 1.0f, 1.0f});
      outRight = math::normalize(math::Vec3{rotationMatrix.m[0], rotationMatrix.m[1], rotationMatrix.m[2]});
      outUp = math::normalize(math::Vec3{rotationMatrix.m[4], rotationMatrix.m[5], rotationMatrix.m[6]});
      if (math::lengthSq(outRight) < 1e-6f) outRight = {1.0f, 0.0f, 0.0f};
      if (math::lengthSq(outUp) < 1e-6f) outUp = {0.0f, 1.0f, 0.0f};
    };

    const auto drawWidgetQuads = [&](const std::vector<HudVert>& verts, bool useTexture, std::uint32_t textureId) {
      if (verts.empty()) return;
      glUseProgram(m_hudProgram);
      const GLint locViewProj = glGetUniformLocation(m_hudProgram, "u_ViewProj");
      if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
      const GLint locUseTex = glGetUniformLocation(m_hudProgram, "u_UseTexture");
      if (locUseTex >= 0) glUniform1i(locUseTex, useTexture ? 1 : 0);
      if (useTexture && textureId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        const GLint locTex = glGetUniformLocation(m_hudProgram, "u_Texture");
        if (locTex >= 0) glUniform1i(locTex, 0);
      }

      glBindVertexArray(m_hudVao);
      glBindBuffer(GL_ARRAY_BUFFER, m_hudVbo);
      if (verts.size() > m_hudCapacityVerts) {
        m_hudCapacityVerts = std::max<std::size_t>(verts.size(), m_hudCapacityVerts * 2 + 256);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_hudCapacityVerts * sizeof(HudVert)), nullptr, GL_DYNAMIC_DRAW);
      }
      glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(HudVert)), verts.data());
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));
      glBindVertexArray(0);
      if (useTexture && textureId != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
      }
      glActiveTexture(GL_TEXTURE0);
    };

    const auto drawBillboardRect = [&](const ecs::systems::GraphicsSystem::FrameSnapshot::BillboardDraw& b,
                                       bool useTexture,
                                       std::uint32_t textureId) {
      math::Vec3 right = camRight;
      math::Vec3 up = camUp;
      if (b.faceMode == ecs::BillboardComponent::FaceMode::YawOnly) {
        math::Vec3 toCamera = frame.camera.position - b.position;
        toCamera.y = 0.0f;
        if (math::lengthSq(toCamera) > 1e-6f) {
          toCamera = math::normalize(toCamera);
          right = math::normalize(math::cross({0.0f, 1.0f, 0.0f}, toCamera));
          if (math::lengthSq(right) < 1e-6f) right = camRight;
        }
        up = {0.0f, 1.0f, 0.0f};
      } else if (b.faceMode == ecs::BillboardComponent::FaceMode::None) {
        axesFromRotation(b.rotation, right, up);
      }

      const float left = -b.pivot.x * b.sizeMeters.x;
      const float rightExtent = (1.0f - b.pivot.x) * b.sizeMeters.x;
      const float down = -b.pivot.y * b.sizeMeters.y;
      const float upExtent = (1.0f - b.pivot.y) * b.sizeMeters.y;
      const math::Vec3 p0 = b.position + right * left + up * upExtent;
      const math::Vec3 p1 = b.position + right * rightExtent + up * upExtent;
      const math::Vec3 p2 = b.position + right * rightExtent + up * down;
      const math::Vec3 p3 = b.position + right * left + up * down;

      std::vector<HudVert> verts;
      verts.reserve(6);
      appendQuad(verts, p0, p1, p2, p3, b.tint, b.tint, b.tint, b.tint, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
      drawWidgetQuads(verts, useTexture, textureId);
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<const ecs::systems::GraphicsSystem::FrameSnapshot::BillboardDraw*> sortedBillboards;
    sortedBillboards.reserve(frame.billboards.size());
    for (const auto& b : frame.billboards) {
      if (!b.enabled || !b.visible) continue;
      sortedBillboards.push_back(&b);
    }
    std::sort(sortedBillboards.begin(), sortedBillboards.end(), [&](const auto* a, const auto* b) {
      const float distA = math::lengthSq(frame.camera.position - a->position);
      const float distB = math::lengthSq(frame.camera.position - b->position);
      return distA > distB;
    });

    for (const auto* billboardPtr : sortedBillboards) {
      const auto& b = *billboardPtr;
      if (!b.enabled || !b.visible) continue;
      const math::Vec3 toCamera = frame.camera.position - b.position;
      const float distSq = math::lengthSq(toCamera);
      if (distSq > b.maxRenderDistance * b.maxRenderDistance) continue;

      if (b.doubleSided) glDisable(GL_CULL_FACE);
      else glEnable(GL_CULL_FACE);

      glDepthMask(b.depthWrite ? GL_TRUE : GL_FALSE);
      const std::uint32_t texId =
          b.textureEnabled ? requestTextureAsset(b.texture, &b.animatedTexture, true, textureTimeSeconds) : 0u;
      drawBillboardRect(b, texId != 0u, texId);
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    // Screen-space 2D elements over the top.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    std::vector<const ecs::systems::GraphicsSystem::FrameSnapshot::Draw2DQuadDraw*> ordered;
    ordered.reserve(frame.draw2d.size());
    for (const auto& q : frame.draw2d) {
      if (!q.enabled) continue;
      ordered.push_back(&q);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) { return a->layer < b->layer; });

    const float fbW = static_cast<float>(m_fbWidth);
    const float fbH = static_cast<float>(m_fbHeight);
    for (const auto* qp : ordered) {
      const auto& q = *qp;

      math::Vec2 topLeft = q.offsetPx;
      switch (q.anchor) {
        case ecs::Draw2DComponent::Anchor::TopLeft: topLeft = q.offsetPx; break;
        case ecs::Draw2DComponent::Anchor::TopRight: topLeft = {fbW - q.offsetPx.x - q.sizePx.x, q.offsetPx.y}; break;
        case ecs::Draw2DComponent::Anchor::BottomLeft: topLeft = {q.offsetPx.x, fbH - q.offsetPx.y - q.sizePx.y}; break;
        case ecs::Draw2DComponent::Anchor::BottomRight:
          topLeft = {fbW - q.offsetPx.x - q.sizePx.x, fbH - q.offsetPx.y - q.sizePx.y};
          break;
        case ecs::Draw2DComponent::Anchor::Center:
          topLeft = {fbW * 0.5f + q.offsetPx.x - q.sizePx.x * 0.5f, fbH * 0.5f + q.offsetPx.y - q.sizePx.y * 0.5f};
          break;
      }

      const math::Vec3 p0 = pixelToNdc(topLeft.x, topLeft.y);
      const math::Vec3 p1 = pixelToNdc(topLeft.x + q.sizePx.x, topLeft.y);
      const math::Vec3 p2 = pixelToNdc(topLeft.x + q.sizePx.x, topLeft.y + q.sizePx.y);
      const math::Vec3 p3 = pixelToNdc(topLeft.x, topLeft.y + q.sizePx.y);

      const std::uint32_t texId = q.textureEnabled ? requestTextureAsset(q.texture, nullptr, true, textureTimeSeconds) : 0u;

      std::vector<HudVert> verts;
      verts.reserve(6);
      appendQuad(verts, p0, p1, p2, p3, q.colorTL, q.colorTR, q.colorBR, q.colorBL, 0.0f, q.uv0.x, q.uv0.y, q.uv1.x, q.uv1.y);
      drawWidgetQuads(verts, texId != 0u, texId);
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
  }
  recordPass("hud_pass", passStart);

  // World-space debug lines.
  passStart = glfwGetTime();
  if ((!frame.rays.empty() || !frame.debugLines.empty()) && m_debugLineProgram && m_debugLineVao && m_debugLineVbo) {
    std::vector<DebugLineVert> verts;
    verts.reserve((frame.rays.size() + frame.debugLines.size()) * 2);
    for (const auto& ray : frame.rays) {
      verts.push_back({ray.start.x, ray.start.y, ray.start.z, ray.color.r, ray.color.g, ray.color.b, ray.color.a});
      verts.push_back({ray.end.x, ray.end.y, ray.end.z, ray.color.r, ray.color.g, ray.color.b, ray.color.a});
    }
    for (const auto& line : frame.debugLines) {
      verts.push_back({line.start.x, line.start.y, line.start.z, line.color.r, line.color.g, line.color.b, line.color.a});
      verts.push_back({line.end.x, line.end.y, line.end.z, line.color.r, line.color.g, line.color.b, line.color.a});
    }

    glUseProgram(m_debugLineProgram);
    const GLint locViewProj = glGetUniformLocation(m_debugLineProgram, "u_ViewProj");
    if (locViewProj >= 0) glUniformMatrix4fv(locViewProj, 1, GL_FALSE, viewProj.m);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if defined(GL_LINE_SMOOTH)
    glEnable(GL_LINE_SMOOTH);
#endif
    glLineWidth(2.0f);

    glBindVertexArray(m_debugLineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_debugLineVbo);
    if (verts.size() > m_debugLineCapacityVerts) {
      m_debugLineCapacityVerts = std::max<std::size_t>(verts.size(), m_debugLineCapacityVerts * 2 + 128);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_debugLineCapacityVerts * sizeof(DebugLineVert)), nullptr,
                   GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(DebugLineVert)), verts.data());
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(verts.size()));
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
#if defined(GL_LINE_SMOOTH)
    glDisable(GL_LINE_SMOOTH);
#endif
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
  }
  recordPass("debug_lines", passStart);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, m_fbWidth, m_fbHeight);

  passStart = glfwGetTime();
  const bool dofOn = frame.camera.depthOfFieldEnabled && frame.camera.dofBlurStrength > 0.0001f;
  const bool blurOn = frame.camera.motionBlurEnabled && frame.camera.motionBlurStrength > 0.0001f && m_hasPrevViewProj;
  if (!dofOn && !blurOn) {
    if (mainFbo != 0) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, sceneW, sceneH, 0, 0, m_fbWidth, m_fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  } else {
    ensurePostTarget(m_postFbo, m_postColorTex, m_postW, m_postH, sceneW, sceneH);

    const auto drawFullscreen = [&](std::uint32_t programId,
                                    std::uint32_t dstFbo,
                                    std::uint32_t srcColor,
                                    std::uint32_t srcDepth,
                                    const std::function<void()>& setUniforms) {
      glBindFramebuffer(GL_FRAMEBUFFER, dstFbo);
      glViewport(0, 0, (dstFbo == 0) ? m_fbWidth : sceneW, (dstFbo == 0) ? m_fbHeight : sceneH);
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
      glDisable(GL_CULL_FACE);
      glDisable(GL_BLEND);
      glUseProgram(programId);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, srcColor);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, srcDepth);

      setUniforms();

      glBindVertexArray(m_skyVao);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindVertexArray(0);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, 0);

      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);
    };

    std::uint32_t curColorTex = m_sceneColorTex;

    if (dofOn) {
      const ShaderService::Program* dofProg = m_shaders.getOrCreate("graphics/shaders/post_dof");
      if (dofProg && dofProg->programId) {
        drawFullscreen(
            dofProg->programId,
            m_postFbo,
            curColorTex,
            m_sceneDepthTex,
            [&]() {
              const GLint locColor = glGetUniformLocation(dofProg->programId, "u_ColorTex");
              const GLint locDepth = glGetUniformLocation(dofProg->programId, "u_DepthTex");
              if (locColor >= 0) glUniform1i(locColor, 0);
              if (locDepth >= 0) glUniform1i(locDepth, 1);
              const GLint locTexel = glGetUniformLocation(dofProg->programId, "u_TexelSize");
              if (locTexel >= 0) glUniform2f(locTexel, 1.0f / static_cast<float>(sceneW), 1.0f / static_cast<float>(sceneH));
              const GLint locNear = glGetUniformLocation(dofProg->programId, "u_Near");
              const GLint locFar = glGetUniformLocation(dofProg->programId, "u_Far");
              if (locNear >= 0) glUniform1f(locNear, frame.camera.nearClip);
              if (locFar >= 0) glUniform1f(locFar, frame.camera.farClip);
              const GLint locFD = glGetUniformLocation(dofProg->programId, "u_FocusDistance");
              const GLint locFR = glGetUniformLocation(dofProg->programId, "u_FocusRange");
              const GLint locBS = glGetUniformLocation(dofProg->programId, "u_BlurStrength");
              if (locFD >= 0) glUniform1f(locFD, frame.camera.dofFocusDistance);
              if (locFR >= 0) glUniform1f(locFR, frame.camera.dofFocusRange);
              if (locBS >= 0) glUniform1f(locBS, frame.camera.dofBlurStrength);
            });
        curColorTex = m_postColorTex;
      }
    }

    if (frame.camera.motionBlurEnabled) {
      const ShaderService::Program* mbProg = m_shaders.getOrCreate("graphics/shaders/post_motion_blur");
      if (mbProg && mbProg->programId) {
        const float strength = m_hasPrevViewProj ? frame.camera.motionBlurStrength : 0.0f;
        drawFullscreen(
            mbProg->programId,
            0,
            curColorTex,
            m_sceneDepthTex,
            [&]() {
              const GLint locColor = glGetUniformLocation(mbProg->programId, "u_ColorTex");
              const GLint locDepth = glGetUniformLocation(mbProg->programId, "u_DepthTex");
              if (locColor >= 0) glUniform1i(locColor, 0);
              if (locDepth >= 0) glUniform1i(locDepth, 1);
              const GLint locTexel = glGetUniformLocation(mbProg->programId, "u_TexelSize");
              if (locTexel >= 0) glUniform2f(locTexel, 1.0f / static_cast<float>(sceneW), 1.0f / static_cast<float>(sceneH));
              const GLint locInv = glGetUniformLocation(mbProg->programId, "u_InvViewProj");
              const GLint locPrev = glGetUniformLocation(mbProg->programId, "u_PrevViewProj");
              if (locInv >= 0) glUniformMatrix4fv(locInv, 1, GL_FALSE, invViewProj.m);
              if (locPrev >= 0) glUniformMatrix4fv(locPrev, 1, GL_FALSE, m_prevViewProj.m);
              const GLint locS = glGetUniformLocation(mbProg->programId, "u_Strength");
              const GLint locMaxPx = glGetUniformLocation(mbProg->programId, "u_MaxBlurPixels");
              const GLint locSamples = glGetUniformLocation(mbProg->programId, "u_Samples");
              if (locS >= 0) glUniform1f(locS, strength);
              if (locMaxPx >= 0) glUniform1f(locMaxPx, frame.camera.motionBlurMaxBlurPixels);
              if (locSamples >= 0) glUniform1i(locSamples, frame.camera.motionBlurSamples);
            });
      } else if (curColorTex != 0 && curColorTex != m_sceneColorTex) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_postFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, sceneW, sceneH, 0, 0, m_fbWidth, m_fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
      } else if (mainFbo != 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, sceneW, sceneH, 0, 0, m_fbWidth, m_fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
      }
    } else if (curColorTex == m_postColorTex) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, m_postFbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, sceneW, sceneH, 0, 0, m_fbWidth, m_fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  }
  recordPass("post_fx", passStart);

  m_prevViewProj = viewProj;
  m_hasPrevViewProj = true;

  // Allow escape to close.
  const float renderMsSoFar = static_cast<float>((glfwGetTime() - renderStart) * 1000.0);

  passStart = glfwGetTime();
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
    if (m_smoothedGpuFrameMs >= 0.0) {
      overlay += "  gpu_ms=" + std::to_string(static_cast<int>(m_smoothedGpuFrameMs + 0.5));
    } else if (m_gpuTimerSupported) {
      overlay += "  gpu_ms=warming";
    } else {
      overlay += "  gpu_ms=n/a";
    }
    if (!extraDebugText.empty()) {
      overlay += "\n";
      overlay += extraDebugText;
    }

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
  recordPass("overlay", passStart);

  endGpuTimerQuery();
  passStart = glfwGetTime();
  glfwSwapBuffers(m_window);
  recordPass("swap_buffers", passStart);
  const auto& renderReport = m_renderDebugger.endFrame();
#undef glGetUniformLocation

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
      if (m_smoothedGpuFrameMs >= 0.0) {
        title += " gpu_ms=" + std::to_string(static_cast<int>(m_smoothedGpuFrameMs + 0.5));
      }
      if (!renderReport.hottestLabel.empty()) {
        title += " hot=" + renderReport.hottestLabel + ":" + std::to_string(static_cast<int>(renderReport.hottestMs + 0.5));
      }
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
