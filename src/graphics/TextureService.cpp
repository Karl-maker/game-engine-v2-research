#include "graphics/TextureService.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

#define GL_SILENCE_DEPRECATION

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#error "Non-Apple builds require an image decoder (e.g. stb_image)."
#endif

namespace graphics {

TextureService::TextureService() = default;

TextureService::~TextureService() { stop(); }

void TextureService::start() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_running) return;
  m_running = true;
  m_worker = std::thread([this] { workerMain(); });
}

void TextureService::stop() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) return;
    m_running = false;
  }
  if (m_worker.joinable()) {
    m_worker.join();
  }

  // GL deletions are owned by the renderer; keep this service simple.
}

std::uint32_t TextureService::requestTexture(const std::string& path, bool srgb) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto& e = m_entries[path];
  if (e.state == Entry::State::Ready) return e.info.glId;
  if (e.state == Entry::State::Failed) return 0;
  if (e.state == Entry::State::Unloaded) {
    e.state = Entry::State::Queued;
    e.info.srgb = srgb;
    m_decodeQueue.push_back(PendingDecode{path, srgb});
  }
  return 0;
}

void TextureService::requestGreenMask(const std::string& path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto& e = m_entries[path];
  e.wantGreenMask = true;
  if (e.greenMask.ready || e.greenMaskQueued) return;
  e.greenMaskQueued = true;
  m_maskDecodeQueue.push_back(PendingMaskDecode{path});
}

std::optional<TextureService::TextureInfo> TextureService::getInfo(const std::string& path) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_entries.find(path);
  if (it == m_entries.end()) return std::nullopt;
  return it->second.info;
}

bool TextureService::hasGreenMaskReady(const std::string& path) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_entries.find(path);
  if (it == m_entries.end()) return false;
  return it->second.greenMask.ready;
}

std::optional<float> TextureService::sampleGreenMask(const std::string& path, float u, float v) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_entries.find(path);
  if (it == m_entries.end()) return std::nullopt;
  const GreenMask& m = it->second.greenMask;
  if (!m.ready || m.width <= 0 || m.height <= 0 || m.green.empty()) return std::nullopt;

  const float uf = u - std::floor(u);
  const float vf = v - std::floor(v);
  const int x = std::clamp(static_cast<int>(uf * static_cast<float>(m.width)), 0, m.width - 1);
  const int y = std::clamp(static_cast<int>(vf * static_cast<float>(m.height)), 0, m.height - 1);
  const std::size_t idx = static_cast<std::size_t>(y) * static_cast<std::size_t>(m.width) + static_cast<std::size_t>(x);
  if (idx >= m.green.size()) return std::nullopt;
  return static_cast<float>(m.green[idx]) / 255.0f;
}

void TextureService::workerMain() {
  for (;;) {
    PendingDecode job;
    PendingMaskDecode maskJob;
    bool doMask = false;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (!m_running) break;
      if (!m_decodeQueue.empty()) {
        job = std::move(m_decodeQueue.back());
        m_decodeQueue.pop_back();
        auto it = m_entries.find(job.path);
        if (it != m_entries.end()) {
          it->second.state = Entry::State::Decoding;
        }
      } else if (!m_maskDecodeQueue.empty()) {
        maskJob = std::move(m_maskDecodeQueue.back());
        m_maskDecodeQueue.pop_back();
        auto it = m_entries.find(maskJob.path);
        if (it != m_entries.end()) {
          it->second.greenMaskQueued = false;
        }
        doMask = true;
      } else {
        job.path.clear();
      }
    }

    if (!doMask && job.path.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      continue;
    }

    if (doMask) {
      std::vector<std::uint8_t> rgba;
      int w = 0;
      int h = 0;
      if (!decodeImageRGBA(maskJob.path, w, h, rgba)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(maskJob.path);
        if (it != m_entries.end()) {
          it->second.greenMask.ready = false;
          it->second.greenMask.green.clear();
        }
        continue;
      }

      constexpr int kMaxLutSize = 128;
      const int lutW = std::max(1, std::min(kMaxLutSize, w));
      const int lutH = std::max(1, std::min(kMaxLutSize, h));
      std::vector<std::uint8_t> lut;
      lut.resize(static_cast<std::size_t>(lutW) * static_cast<std::size_t>(lutH));
      for (int y = 0; y < lutH; ++y) {
        const int srcY = (y * h) / lutH;
        for (int x = 0; x < lutW; ++x) {
          const int srcX = (x * w) / lutW;
          const std::size_t si = (static_cast<std::size_t>(srcY) * static_cast<std::size_t>(w) + static_cast<std::size_t>(srcX)) * 4u;
          const std::size_t di = static_cast<std::size_t>(y) * static_cast<std::size_t>(lutW) + static_cast<std::size_t>(x);
          lut[di] = (si + 1u < rgba.size()) ? rgba[si + 1u] : 0u;  // green channel
        }
      }

      std::lock_guard<std::mutex> lock(m_mutex);
      auto it = m_entries.find(maskJob.path);
      if (it != m_entries.end()) {
        it->second.greenMask.width = lutW;
        it->second.greenMask.height = lutH;
        it->second.greenMask.green = std::move(lut);
        it->second.greenMask.ready = true;
      }
      continue;
    }

    PendingUpload up;
    up.path = job.path;
    up.srgb = job.srgb;
    if (!decodeImageRGBA(job.path, up.width, up.height, up.rgba)) {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto it = m_entries.find(job.path);
      if (it != m_entries.end()) it->second.state = Entry::State::Failed;
      continue;
    }

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_uploadQueue.push_back(std::move(up));
    }
  }
}

bool TextureService::decodeImageRGBA(const std::string& path,
                                     int& outW,
                                     int& outH,
                                     std::vector<std::uint8_t>& outRgba) const {
#ifdef __APPLE__
  CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
  if (!cfPath) return false;
  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, false);
  CFRelease(cfPath);
  if (!url) return false;

  CGImageSourceRef src = CGImageSourceCreateWithURL(url, nullptr);
  CFRelease(url);
  if (!src) return false;

  CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, nullptr);
  CFRelease(src);
  if (!img) return false;

  const std::size_t w = CGImageGetWidth(img);
  const std::size_t h = CGImageGetHeight(img);
  if (w == 0 || h == 0) {
    CGImageRelease(img);
    return false;
  }

  outW = static_cast<int>(w);
  outH = static_cast<int>(h);
  outRgba.resize(w * h * 4);

  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  const CGBitmapInfo bi = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
  CGContextRef ctx =
      CGBitmapContextCreate(outRgba.data(), w, h, 8, w * 4, cs, bi);
  CGColorSpaceRelease(cs);

  if (!ctx) {
    CGImageRelease(img);
    return false;
  }

  CGContextClearRect(ctx, CGRectMake(0, 0, static_cast<CGFloat>(w), static_cast<CGFloat>(h)));
  CGContextDrawImage(ctx, CGRectMake(0, 0, static_cast<CGFloat>(w), static_cast<CGFloat>(h)), img);
  CGContextRelease(ctx);
  CGImageRelease(img);

  return true;
#else
  int w = 0, h = 0, comp = 0;
  stbi_uc* data = stbi_load(path.c_str(), &w, &h, &comp, 4);
  if (!data) {
    std::cerr << "stbi_load failed for " << path << "\n";
    return false;
  }
  outW = w;
  outH = h;
  outRgba.assign(data, data + (w * h * 4));
  stbi_image_free(data);
  return true;
#endif
}

void TextureService::flushUploads(std::size_t maxUploadsPerFrame) {
  std::vector<PendingUpload> local;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::size_t n = std::min(maxUploadsPerFrame, m_uploadQueue.size());
    local.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      local.push_back(std::move(m_uploadQueue.back()));
      m_uploadQueue.pop_back();
    }
  }

  for (auto& up : local) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const GLint internal = up.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internal,
                 up.width,
                 up.height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 up.rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(up.path);
    if (it == m_entries.end()) continue;
    it->second.state = Entry::State::Ready;
    it->second.info.glId = tex;
    it->second.info.width = up.width;
    it->second.info.height = up.height;
    it->second.info.srgb = up.srgb;
    it->second.info.ready = true;
  }
}

void TextureService::destroyAllGlTextures() {
  std::vector<GLuint> ids;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [_, e] : m_entries) {
      if (e.state == Entry::State::Ready && e.info.glId != 0) {
        ids.push_back(static_cast<GLuint>(e.info.glId));
        e.info.glId = 0;
        e.info.ready = false;
      }
      if (e.state == Entry::State::Ready) {
        e.state = Entry::State::Unloaded;
      }
    }
  }
  if (!ids.empty()) {
    glDeleteTextures(static_cast<GLsizei>(ids.size()), ids.data());
  }
}

}  // namespace graphics
