#pragma once

// Author: Karl-Johan Bailey
//
// NoiseConfig (descriptive)
// Shared noise settings for terrain/world generation and other procedural systems.

#include <cstdint>

namespace terrain {

enum class NoiseType {
  Perlin,
  Value,
};

struct NoiseConfig {
  NoiseType type = NoiseType::Perlin;
  std::uint32_t seed = 12345;

  // Typical controls.
  float frequency = 0.01f;
  int octaves = 4;
  float lacunarity = 2.0f;
  float persistence = 0.5f;
};

}  // namespace terrain

