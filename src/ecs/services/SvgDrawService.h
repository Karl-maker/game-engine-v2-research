#pragma once

// Author: Karl-Johan Bailey
//
// SvgDrawService (utility)
// Lightweight helper for working with SVG assets in the demo renderer.
//
// Current behavior:
// - Parses intrinsic SVG dimensions (width/height or viewBox) to compute aspect ratio.
// - Produces sizing helpers for screen-space quads (px) and world billboards (meters).
// - Rendering is still texture-driven: TextureService requests the SVG path and ImageIO decodes it.
//
// Future extensions can add true path-to-geometry conversion (coords/triangulation) without
// changing gameplay-facing HUD components.

#include "math/Vec2.h"
#include "render/AssetRef.h"

#include <optional>
#include <string>

namespace ecs::services {

struct SvgAssetInfo final {
  std::string path;
  float width = 0.0f;
  float height = 0.0f;
  float aspect = 1.0f;  // width / height
  bool valid = false;
};

struct SvgSizeRequest final {
  // Optional absolute sizing. If omitted, uses the SVG intrinsic size.
  std::optional<float> targetHeightPx;
  std::optional<float> targetHeightMeters;

  // Multiplicative scale applied after choosing a base size.
  float scale = 1.0f;
};

struct SvgSizeResult final {
  math::Vec2 sizePx{0.0f, 0.0f};
  math::Vec2 sizeMeters{0.0f, 0.0f};
  float aspect = 1.0f;
  bool valid = false;
};

class SvgDrawService final {
 public:
  // Parses SVG metadata (width/height/viewBox) to compute intrinsic aspect ratio.
  static SvgAssetInfo inspect(const std::string& svgPath);

  // Convenience: generate an AssetRef pointing at a renderable texture for an SVG.
  // On macOS, SVG decoding support can vary; this method may rasterize the SVG to a cached PNG via `qlmanage`.
  static render::AssetRef textureRef(const std::string& svgPath, int rasterHeightPx = 512);

  // Computes px/meters sizes based on intrinsic aspect ratio + the provided request.
  static SvgSizeResult computeSize(const std::string& svgPath, const SvgSizeRequest& req);
};

}  // namespace ecs::services
