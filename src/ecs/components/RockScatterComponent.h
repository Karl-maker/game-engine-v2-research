#pragma once

// Author: Karl-Johan Bailey
//
// RockScatterComponent (descriptive only)
// Describes a procedural scatter of small rocks/pebbles for a renderer/system to generate.

#include "math/Vec3.h"

#include <cstdint>
#include <string>

namespace ecs {

struct RockScatterComponent {
  bool enabled = true;

  // Scatter area in XZ (x=width, z=depth), centered on the entity transform.
  math::Vec3 area{12.0f, 0.0f, 12.0f};

  // Instances per square meter (system-defined).
  float density = 1.6f;

  std::uint32_t seed = 424242;
  std::string distribution = "clumped";

  // Size range in meters.
  float minScale = 0.04f;
  float maxScale = 0.12f;

  // Clump profile controls (system-defined).
  float clumpiness = 0.8f;  // 0..1
  float patchScale = 0.06f; // world-space noise frequency-ish

  bool castShadows = false;
  bool receiveShadows = true;
  float lodBias = 1.0f;
};

}  // namespace ecs

