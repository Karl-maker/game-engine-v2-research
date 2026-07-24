#pragma once

// Author: Karl-Johan Bailey
//
// FogVolumeComponent (descriptive)
// Defines a simple axis-aligned fog/mist volume for the renderer to apply.
// The graphics system collects these and the renderer applies fog in shaders.

#include "math/Vec3.h"
#include "render/Color.h"

namespace ecs {

struct FogVolumeComponent final {
  bool enabled = true;

  // Axis-aligned box volume centered at TransformComponent::position.
  // Interpreted as full size in meters.
  math::Vec3 sizeMeters{100.0f, 50.0f, 100.0f};

  // Visuals.
  render::Color color{0.20f, 0.28f, 0.22f, 1.0f};

  // Distance-based fog (exponential-ish).
  float density = 0.012f;        // higher = thicker
  float startDistance = 18.0f;   // meters (no fog before)
  float endDistance = 220.0f;    // meters (near full fog after)

  // Height shaping inside the volume.
  float heightFalloff = 0.06f;   // higher = fog hugs the ground more
  float baseHeightOffset = 0.0f; // meters, shifts the "ground" reference
};

}  // namespace ecs

