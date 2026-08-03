#pragma once

// Author: Karl-Johan Bailey
//
// CombatantHudComponent (descriptive only)
// Marks a world-space HUD entity that follows a combatant and renders a camera-facing textured billboard.

#include "ecs/EntityId.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "render/Color.h"

#include <string>

namespace ecs {

struct CombatantHudComponent final {
  bool enabled = true;

  // Gameplay entity whose stats this HUD should reflect.
  EntityId targetEntity = kInvalidEntityId;

  // Base texture used for this HUD.
  std::string texturePath = "assets/hud/combatant-health.png";

  // Attachment offset relative to the combatant (meters).
  math::Vec3 worldOffset{0.0f, 2.25f, 0.0f};

  // World-space height of the billboard (meters). Width is derived from the texture aspect ratio.
  float heightMeters = 0.22f;

  // Culling distance for the base + fill billboards.
  float maxRenderDistanceMeters = 180.0f;

  // Subtle distance scaling (1.0 at start, ramps to distanceScaleAtEnd at end).
  bool distanceScaleEnabled = true;
  float distanceScaleStartMeters = 8.0f;
  float distanceScaleEndMeters = 42.0f;
  float distanceScaleAtEnd = 1.20f;

  // Overall tint applied to the HUD texture.
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};

  // Optional dynamic fill quad.
  bool fillEnabled = true;
  render::Color fillColor{0.10f, 0.95f, 0.25f, 0.85f};
  float fillWidthRatio = 0.84f;
  float fillHeightRatio = 0.28f;
  float fillDepthBiasMeters = 0.015f;

  // Internal: spawned fill entity id (managed by CombatantHudSystem).
  EntityId fillEntity = kInvalidEntityId;
};

}  // namespace ecs
