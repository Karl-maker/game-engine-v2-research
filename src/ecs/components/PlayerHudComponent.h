#pragma once

// Author: Karl-Johan Bailey
//
// PlayerHudComponent (descriptive only)
// Marks a screen-space HUD entity for the local player.

#include "ecs/EntityId.h"
#include "render/Color.h"

#include <string>

namespace ecs {

struct PlayerHudComponent final {
  bool enabled = true;

  // Gameplay entity whose stats this HUD should reflect.
  EntityId targetEntity = kInvalidEntityId;

  // SVG to draw as the base player HUD element.
  std::string svgPath = "assets/hud/player-health.svg";

  // Desired on-screen height in pixels. Width is derived from SVG aspect ratio.
  float heightPx = 96.0f;

  // Bottom-left padding in pixels.
  float marginLeftPx = 24.0f;
  float marginBottomPx = 24.0f;

  // Base SVG tint.
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
};

}  // namespace ecs

