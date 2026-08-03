#pragma once

// Author: Karl-Johan Bailey
//
// ImageDrawService (utility)
// Minimal helpers for image-backed HUD sizing (PNG only for now).

#include "math/Vec2.h"
#include "render/AssetRef.h"

#include <optional>
#include <string>

namespace ecs::services {

struct ImageAssetInfo final {
  std::string path;
  int widthPx = 0;
  int heightPx = 0;
  float aspect = 1.0f;  // width / height
  bool valid = false;
};

struct ImageSizeRequest final {
  std::optional<float> targetHeightPx;
  std::optional<float> targetWidthPx;
  std::optional<float> targetHeightMeters;

  float scale = 1.0f;
};

struct ImageSizeResult final {
  math::Vec2 sizePx{0.0f, 0.0f};
  math::Vec2 sizeMeters{0.0f, 0.0f};
  float aspect = 1.0f;
  bool valid = false;
};

class ImageDrawService final {
 public:
  // Inspects PNG IHDR to get width/height (no full decode).
  static ImageAssetInfo inspectPng(const std::string& pngPath);

  // Convenience: asset ref for a texture path.
  static render::AssetRef textureRef(const std::string& texturePath);

  // Computes sizes while keeping aspect ratio when possible.
  static ImageSizeResult computeSize(const std::string& texturePath, const ImageSizeRequest& req);
};

}  // namespace ecs::services

