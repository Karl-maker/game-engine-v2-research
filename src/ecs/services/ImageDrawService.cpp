// Author: Karl-Johan Bailey

#include "ecs/services/ImageDrawService.h"

#include <array>
#include <cstdint>
#include <fstream>

namespace ecs::services {

namespace {

static std::uint32_t readBeU32(const std::array<std::uint8_t, 4>& b) {
  return (static_cast<std::uint32_t>(b[0]) << 24) | (static_cast<std::uint32_t>(b[1]) << 16) | (static_cast<std::uint32_t>(b[2]) << 8) |
         (static_cast<std::uint32_t>(b[3]) << 0);
}

}  // namespace

ImageAssetInfo ImageDrawService::inspectPng(const std::string& pngPath) {
  ImageAssetInfo info;
  info.path = pngPath;

  std::ifstream file(pngPath, std::ios::binary);
  if (!file) return info;

  // PNG signature (8 bytes).
  const std::array<std::uint8_t, 8> expectedSig{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  std::array<std::uint8_t, 8> sig{};
  file.read(reinterpret_cast<char*>(sig.data()), static_cast<std::streamsize>(sig.size()));
  if (!file || sig != expectedSig) return info;

  // First chunk should be IHDR: length(4) + type(4) + data(13) + crc(4).
  std::array<std::uint8_t, 4> lengthBytes{};
  std::array<std::uint8_t, 4> typeBytes{};
  file.read(reinterpret_cast<char*>(lengthBytes.data()), 4);
  file.read(reinterpret_cast<char*>(typeBytes.data()), 4);
  if (!file) return info;

  const std::uint32_t length = readBeU32(lengthBytes);
  const std::array<std::uint8_t, 4> expectedType{'I', 'H', 'D', 'R'};
  if (typeBytes != expectedType) return info;
  if (length < 8) return info;

  std::array<std::uint8_t, 4> wBytes{};
  std::array<std::uint8_t, 4> hBytes{};
  file.read(reinterpret_cast<char*>(wBytes.data()), 4);
  file.read(reinterpret_cast<char*>(hBytes.data()), 4);
  if (!file) return info;

  const std::uint32_t w = readBeU32(wBytes);
  const std::uint32_t h = readBeU32(hBytes);
  if (w == 0 || h == 0) return info;

  info.widthPx = static_cast<int>(w);
  info.heightPx = static_cast<int>(h);
  info.aspect = static_cast<float>(w) / static_cast<float>(h);
  info.valid = true;
  return info;
}

render::AssetRef ImageDrawService::textureRef(const std::string& texturePath) { return {true, texturePath, 0}; }

ImageSizeResult ImageDrawService::computeSize(const std::string& texturePath, const ImageSizeRequest& req) {
  ImageSizeResult out;
  const auto info = inspectPng(texturePath);
  if (!info.valid) return out;
  out.aspect = info.aspect;

  if (req.targetHeightPx.has_value()) {
    const float h = std::max(0.0f, *req.targetHeightPx) * req.scale;
    out.sizePx = {h * info.aspect, h};
    out.valid = true;
  } else if (req.targetWidthPx.has_value()) {
    const float w = std::max(0.0f, *req.targetWidthPx) * req.scale;
    out.sizePx = {w, w / info.aspect};
    out.valid = true;
  }

  if (req.targetHeightMeters.has_value()) {
    const float h = std::max(0.0f, *req.targetHeightMeters) * req.scale;
    out.sizeMeters = {h * info.aspect, h};
    out.valid = true;
  }

  return out;
}

}  // namespace ecs::services

