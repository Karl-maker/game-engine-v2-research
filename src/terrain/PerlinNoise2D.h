#pragma once

// Author: Karl-Johan Bailey
//
// PerlinNoise2D (service)
// Deterministic improved Perlin noise implementation.

#include "terrain/INoise2D.h"
#include "terrain/NoiseConfig.h"

#include <array>
#include <cstdint>

namespace terrain {

class PerlinNoise2D final : public INoise2D {
 public:
  explicit PerlinNoise2D(std::uint32_t seed);
  float sample(float x, float y) const override;

  // Fractal (octaves) helper using a NoiseConfig.
  float sampleFractal(float x, float y, const NoiseConfig& cfg) const;

 private:
  std::array<std::uint8_t, 512> m_perm{};
};

}  // namespace terrain

