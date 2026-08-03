#pragma once

// Author: Karl-Johan Bailey
//
// PlayerHudComponent (descriptive only)
// Marks a screen-space HUD entity for the local player.

#include "ecs/EntityId.h"
#include "math/Vec2.h"
#include "render/Color.h"

#include <string>

namespace ecs {

struct PlayerHudComponent final {
  bool enabled = true;

  // Gameplay entity whose stats this HUD should reflect.
  EntityId targetEntity = kInvalidEntityId;

  // Base texture to draw for the player HUD.
  std::string texturePath = "assets/hud/player-health.png";

  // Desired on-screen height in pixels. Width is derived from the texture aspect ratio.
  float heightPx = 96.0f;

  // Bottom-left padding in pixels.
  float marginLeftPx = 24.0f;
  float marginBottomPx = 24.0f;

  // Base texture tint.
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};

  // Texture UV flipping (useful if the asset loads "upside down").
  bool flipU = false;
  bool flipV = false;

  // Optional dynamic fill quad drawn on top (screen-space).
  bool fillEnabled = true;
  int fillLayer = 1;
  render::Color fillColor{0.10f, 0.95f, 0.25f, 0.90f};
  float fillWidthRatio = 0.84f;   // fraction of base width usable for fill
  float fillHeightRatio = 0.28f;  // fraction of base height usable for fill
  math::Vec2 fillOffsetPx{24.0f, 28.0f};  // additional offset from the HUD's bottom-left (pixels)
  bool fillFromRight = false;
};

}  // namespace ecs
