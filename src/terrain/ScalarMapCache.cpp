#include "terrain/ScalarMapCache.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <cmath>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

namespace terrain {

static float wrap01(float x) { return x - std::floor(x); }

const ScalarMapCache::Map* ScalarMapCache::getOrLoadGrayscale(const std::string& path, int maxSize) {
  if (path.empty()) return nullptr;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(path);
    if (it != m_cache.end()) return &it->second;
  }

  const std::optional<Map> loaded = loadGrayscale(path, maxSize);
  if (!loaded.has_value()) return nullptr;

  std::lock_guard<std::mutex> lock(m_mutex);
  auto [it, _] = m_cache.emplace(path, *loaded);
  return &it->second;
}

float ScalarMapCache::sample01(const Map& m, float u, float v, int mip) {
  if (m.mips.empty()) return 0.0f;
  mip = std::max(0, std::min(mip, static_cast<int>(m.mips.size()) - 1));
  const int w = std::max(1, m.width >> mip);
  const int h = std::max(1, m.height >> mip);
  const auto& img = m.mips[static_cast<std::size_t>(mip)];
  if (w <= 0 || h <= 0 || img.empty()) return 0.0f;

  const float uf = wrap01(u);
  const float vf = wrap01(v);

  const float x = uf * static_cast<float>(w - 1);
  const float y = vf * static_cast<float>(h - 1);

  const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, w - 1);
  const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, h - 1);
  const int x1 = std::clamp(x0 + 1, 0, w - 1);
  const int y1 = std::clamp(y0 + 1, 0, h - 1);

  const float tx = x - static_cast<float>(x0);
  const float ty = y - static_cast<float>(y0);

  const auto at = [&](int px, int py) -> float {
    const std::size_t idx = static_cast<std::size_t>(py) * static_cast<std::size_t>(w) + static_cast<std::size_t>(px);
    if (idx >= img.size()) return 0.0f;
    return static_cast<float>(img[idx]) / 255.0f;
  };

  const float a = at(x0, y0);
  const float b = at(x1, y0);
  const float c = at(x0, y1);
  const float d = at(x1, y1);

  const float ab = a + (b - a) * tx;
  const float cd = c + (d - c) * tx;
  return ab + (cd - ab) * ty;
}

std::optional<ScalarMapCache::Map> ScalarMapCache::loadGrayscale(const std::string& path, int maxSize) {
#ifndef __APPLE__
  (void)path;
  (void)maxSize;
  return std::nullopt;
#else
  const int maxS = std::max(1, maxSize);

  CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
  if (!cfPath) return std::nullopt;
  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, false);
  CFRelease(cfPath);
  if (!url) return std::nullopt;

  CGImageSourceRef src = CGImageSourceCreateWithURL(url, nullptr);
  CFRelease(url);
  if (!src) return std::nullopt;

  CGImageRef image = CGImageSourceCreateImageAtIndex(src, 0, nullptr);
  CFRelease(src);
  if (!image) return std::nullopt;

  const std::size_t srcW = CGImageGetWidth(image);
  const std::size_t srcH = CGImageGetHeight(image);
  if (srcW == 0 || srcH == 0) {
    CGImageRelease(image);
    return std::nullopt;
  }

  const float scaleW = static_cast<float>(maxS) / static_cast<float>(srcW);
  const float scaleH = static_cast<float>(maxS) / static_cast<float>(srcH);
  const float scale = std::min(1.0f, std::min(scaleW, scaleH));
  const int outW = std::max(1, static_cast<int>(std::round(static_cast<float>(srcW) * scale)));
  const int outH = std::max(1, static_cast<int>(std::round(static_cast<float>(srcH) * scale)));

  CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
  if (!cs) {
    CGImageRelease(image);
    return std::nullopt;
  }

  const std::size_t bytesPerRow = static_cast<std::size_t>(outW);
  std::vector<std::uint8_t> pixels;
  pixels.resize(static_cast<std::size_t>(outW) * static_cast<std::size_t>(outH));

  CGContextRef ctx = CGBitmapContextCreate(pixels.data(),
                                          outW,
                                          outH,
                                          8,
                                          bytesPerRow,
                                          cs,
                                          static_cast<std::uint32_t>(kCGImageAlphaNone));
  CGColorSpaceRelease(cs);
  if (!ctx) {
    CGImageRelease(image);
    return std::nullopt;
  }

  // Draw scaled into the output buffer.
  CGContextSetBlendMode(ctx, kCGBlendModeCopy);
  CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
  CGContextDrawImage(ctx, CGRectMake(0, 0, outW, outH), image);
  CGContextRelease(ctx);
  CGImageRelease(image);

  Map out{};
  out.width = outW;
  out.height = outH;
  out.mips.clear();
  out.mips.push_back(std::move(pixels));
  buildMips(out);
  return out;
#endif
}

void ScalarMapCache::buildMips(Map& out) {
  if (out.mips.empty() || out.width <= 0 || out.height <= 0) return;

  int w = out.width;
  int h = out.height;
  for (;;) {
    if (w <= 1 && h <= 1) break;
    const int nw = std::max(1, w / 2);
    const int nh = std::max(1, h / 2);
    const auto& src = out.mips.back();
    std::vector<std::uint8_t> dst;
    dst.resize(static_cast<std::size_t>(nw) * static_cast<std::size_t>(nh));

    auto srcAt = [&](int x, int y) -> std::uint8_t {
      x = std::clamp(x, 0, w - 1);
      y = std::clamp(y, 0, h - 1);
      const std::size_t idx = static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x);
      return (idx < src.size()) ? src[idx] : 0u;
    };

    for (int y = 0; y < nh; ++y) {
      for (int x = 0; x < nw; ++x) {
        const int sx = x * 2;
        const int sy = y * 2;
        const int sum = static_cast<int>(srcAt(sx + 0, sy + 0)) +
                        static_cast<int>(srcAt(sx + 1, sy + 0)) +
                        static_cast<int>(srcAt(sx + 0, sy + 1)) +
                        static_cast<int>(srcAt(sx + 1, sy + 1));
        const std::size_t di = static_cast<std::size_t>(y) * static_cast<std::size_t>(nw) + static_cast<std::size_t>(x);
        dst[di] = static_cast<std::uint8_t>(std::clamp(sum / 4, 0, 255));
      }
    }

    out.mips.push_back(std::move(dst));
    w = nw;
    h = nh;
  }
}

}  // namespace terrain

