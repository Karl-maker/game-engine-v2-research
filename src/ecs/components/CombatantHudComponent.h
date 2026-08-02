#pragma once

// Author: Karl-Johan Bailey
//
// CombatantHudComponent (descriptive only)
// Marks a world-space HUD entity that follows a combatant and renders a camera-facing SVG billboard.

#include "ecs/EntityId.h"
#include "math/Vec3.h"
#include "render/Color.h"

#include <string>

namespace ecs {

struct CombatantHudComponent final {
  bool enabled = true;

  // Gameplay entity whose stats this HUD should reflect.
  EntityId targetEntity = kInvalidEntityId;

  // Default SVG used for this HUD.
  std::string svgPath = "assets/hud/combatant-health.svg";

  // Attachment offset relative to the combatant (meters).
  math::Vec3 worldOffset{0.0f, 2.25f, 0.0f};

  // World-space height of the billboard (meters). Width is derived from SVG aspect ratio.
  float heightMeters = 0.55f;

  // Overall tint applied to the SVG texture.
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
};

}  // namespace ecs
