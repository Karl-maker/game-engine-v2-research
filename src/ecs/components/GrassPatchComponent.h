#pragma once

// Author: Karl-Johan Bailey
//
// GrassPatchComponent (descriptive only)
// Describes a procedural grass patch for another system to generate/render.
// No mesh references are stored here; the renderer/foliage system decides assets.

#include "math/Vec3.h"

#include <cstdint>
#include <string>

namespace ecs {

struct GrassPatchComponent {
  bool enabled = true;

  // Patch area (system-defined interpretation).
  // Common pattern: x=width, y=unused, z=depth for an axis-aligned rectangle in XZ.
  math::Vec3 area{10.0f, 0.0f, 10.0f};

  // Instances per square meter (or engine-defined units).
  float density = 8.0f;

  std::uint32_t seed = 12345;

  // Distribution profile (system-defined). Examples: "uniform", "clumped", "poisson".
  std::string distribution = "poisson";

  // Grass type key used by a foliage library/system (engine-defined).
  // Examples: "field_grass_short", "savanna_tuft", "jungle_blade".
  std::string grassType = "field_grass";

  // Instance data generation hints.
  bool generateInstanceData = true;
  float minScale = 0.8f;
  float maxScale = 1.2f;
  float jitter = 1.0f;  // randomness strength (system-defined)

  // Render settings hints (engine-defined).
  bool castShadows = false;
  bool receiveShadows = true;
  float lodBias = 1.0f;
};

}  // namespace ecs

