#pragma once

// Author: Karl-Johan Bailey
//
// Draw2DComponent (descriptive only)
// Describes simple 2D quads to draw on top of the frame (screen-space).
//
// Notes:
// - This component is intentionally descriptive only; systems decide how to update it.
// - Renderer interprets quads in pixel units and converts them to clip-space.
// - Supports per-corner colors for simple gradients and optional textures.

#include "math/Vec2.h"
#include "render/AssetRef.h"
#include "render/Color.h"

#include <string>
#include <vector>

namespace ecs {

struct Draw2DComponent final {
  enum class Anchor {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center,
  };

  struct Quad final {
    bool enabled = true;
    std::string name;
    int layer = 0;  // lower draws first; higher draws on top

    Anchor anchor = Anchor::TopLeft;
    math::Vec2 offsetPx{0.0f, 0.0f};  // offset from the anchor in pixels
    math::Vec2 sizePx{64.0f, 64.0f};

    bool textureEnabled = false;
    render::AssetRef texture{};
    math::Vec2 uv0{0.0f, 0.0f};
    math::Vec2 uv1{1.0f, 1.0f};

    // Per-corner colors (top-left, top-right, bottom-right, bottom-left).
    render::Color colorTL{1.0f, 1.0f, 1.0f, 1.0f};
    render::Color colorTR{1.0f, 1.0f, 1.0f, 1.0f};
    render::Color colorBR{1.0f, 1.0f, 1.0f, 1.0f};
    render::Color colorBL{1.0f, 1.0f, 1.0f, 1.0f};
  };

  bool enabled = true;
  std::vector<Quad> quads;
};

}  // namespace ecs

