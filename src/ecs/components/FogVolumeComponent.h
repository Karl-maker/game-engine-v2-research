#pragma once

// Author: Karl-Johan Bailey
//
// FogVolumeComponent (descriptive)
// Defines scene-wide atmospheric mist settings for the renderer.
// The graphics system collects the first enabled component and shaders apply it
// as layered distance fog with ground-hugging height falloff.

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "render/Color.h"

namespace ecs {

struct FogVolumeComponent final {
  bool enabled = true;

  // Scene-wide atmospheric mist. TransformComponent::position anchors the
  // ground reference and noise field; position.y + baseHeightOffset acts as
  // the fog "floor".
  render::Color color{0.62f, 0.69f, 0.76f, 1.0f};

  // Distance shaping.
  float density = 0.028f;            // higher = thicker at distance
  float startDistance = 6.0f;        // meters (keep near field readable)
  float endDistance = 110.0f;        // meters (mostly obscured past this)
  float maxOpacity = 0.92f;          // clamp so silhouettes still read
  float distanceExponent = 1.35f;    // >1 pushes thickness farther away

  // Height shaping.
  float heightFalloff = 0.085f;      // higher = fog hugs the ground more
  float baseHeightOffset = -4.0f;    // meters relative to transform.y
  float horizonStrength = 0.26f;     // thicker when looking across the ground

  // Large and fine mist breakup.
  float noiseScale = 0.028f;
  float noiseStrength = 0.42f;
  float detailNoiseScale = 0.095f;
  float detailNoiseStrength = 0.18f;
  math::Vec2 windDirection{1.0f, 0.35f};
  float windSpeed = 0.75f;
};

}  // namespace ecs
