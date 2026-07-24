#pragma once

// Author: Karl-Johan Bailey
//
// HudComponent (descriptive only)
// Describes screen-space or world-space HUD widgets.
// A widget can be a panel, image, text label, or progress bar.

#include "ecs/EntityId.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "render/AnimatedTexture.h"
#include "render/AssetRef.h"
#include "render/Color.h"

#include <string>
#include <vector>

namespace ecs {

struct HudComponent final {
  enum class Space {
    Screen,
    World,
  };

  enum class Kind {
    Panel,
    Image,
    Text,
    Bar,
  };

  enum class ValueSource {
    Manual,
    StatsHealth,
  };

  struct Widget final {
    bool enabled = true;
    std::string name;
    Space space = Space::Screen;
    Kind kind = Kind::Panel;
    ValueSource valueSource = ValueSource::Manual;

    // Screen-space widgets use pixels; world-space widgets use meters.
    math::Vec2 positionPx{24.0f, 24.0f};
    math::Vec2 sizePx{220.0f, 28.0f};
    math::Vec3 worldOffset{0.0f, 2.0f, 0.0f};
    math::Vec2 sizeMeters{1.0f, 0.25f};
    bool billboard = true;

    EntityId sourceEntity = kInvalidEntityId;
    float value = 0.0f;
    float maxValue = 100.0f;
    bool showValueText = false;

    std::string label;
    std::string text;
    float textScalePx = 16.0f;

    bool showBackground = true;
    bool showBorder = false;
    float borderThicknessPx = 2.0f;

    render::AssetRef texture{};
    render::AnimatedTexture animatedTexture{};
    bool textureEnabled = false;

    render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
    render::Color backgroundColor{0.08f, 0.08f, 0.08f, 0.80f};
    render::Color borderColor{1.0f, 1.0f, 1.0f, 1.0f};
    render::Color fillColor{0.85f, 0.15f, 0.15f, 1.0f};
    render::Color fillBackgroundColor{0.18f, 0.18f, 0.18f, 0.85f};
    render::Color textColor{1.0f, 1.0f, 1.0f, 1.0f};
  };

  bool enabled = true;
  std::vector<Widget> widgets;
};

}  // namespace ecs
