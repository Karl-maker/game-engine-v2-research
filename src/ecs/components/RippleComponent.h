#pragma once

// Author: Karl-Johan Bailey
//
// RippleComponent (descriptive only)
// Describes a localized water ripple emitter layered on top of the base water shader.

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "render/AssetRef.h"

namespace ecs {

struct RippleComponent final {
  bool enabled = true;

  float radiusMeters = 8.0f;
  float lengthMeters = 10.0f;
  float widthMeters = 4.0f;
  float strength = 0.35f;
  float magnitude = 1.0f;
  float frequency = 8.5f;
  float speed = 2.2f;
  float falloffPower = 1.8f;
  float tiling = 0.22f;
  math::Vec2 direction{1.0f, 0.0f};
  float driftSpeed = 0.12f;
  float foamBoost = 0.20f;
  float noiseScale = 0.28f;
  float noiseStrength = 0.35f;
  float noiseSpeed = 0.55f;

  bool textureEnabled = false;
  render::AssetRef texture{};
};

}  // namespace ecs
