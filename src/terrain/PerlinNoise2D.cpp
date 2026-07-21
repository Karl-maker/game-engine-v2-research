#include "terrain/PerlinNoise2D.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <cmath>
#include <random>

namespace terrain {

static float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static float grad(int hash, float x, float y) {
  // 8 gradient directions.
  switch (hash & 7) {
    case 0: return x + y;
    case 1: return -x + y;
    case 2: return x - y;
    case 3: return -x - y;
    case 4: return x;
    case 5: return -x;
    case 6: return y;
    default: return -y;
  }
}

PerlinNoise2D::PerlinNoise2D(std::uint32_t seed) {
  std::array<std::uint8_t, 256> p{};
  for (int i = 0; i < 256; ++i) p[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i);

  std::mt19937 rng(seed);
  std::shuffle(p.begin(), p.end(), rng);

  for (int i = 0; i < 256; ++i) {
    m_perm[static_cast<std::size_t>(i)] = p[static_cast<std::size_t>(i)];
    m_perm[static_cast<std::size_t>(i + 256)] = p[static_cast<std::size_t>(i)];
  }
}

float PerlinNoise2D::sample(float x, float y) const {
  const int xi = static_cast<int>(std::floor(x)) & 255;
  const int yi = static_cast<int>(std::floor(y)) & 255;

  const float xf = x - std::floor(x);
  const float yf = y - std::floor(y);

  const float u = fade(xf);
  const float v = fade(yf);

  const int aa = m_perm[static_cast<std::size_t>(m_perm[static_cast<std::size_t>(xi)] + yi)];
  const int ab = m_perm[static_cast<std::size_t>(m_perm[static_cast<std::size_t>(xi)] + yi + 1)];
  const int ba = m_perm[static_cast<std::size_t>(m_perm[static_cast<std::size_t>(xi + 1)] + yi)];
  const int bb = m_perm[static_cast<std::size_t>(m_perm[static_cast<std::size_t>(xi + 1)] + yi + 1)];

  const float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0f, yf), u);
  const float x2 = lerp(grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f), u);
  return lerp(x1, x2, v);  // ~[-1, 1]
}

float PerlinNoise2D::sampleFractal(float x, float y, const NoiseConfig& cfg) const {
  float amplitude = 1.0f;
  float frequency = cfg.frequency;
  float sum = 0.0f;
  float ampSum = 0.0f;

  const int octaves = std::max(1, cfg.octaves);
  for (int i = 0; i < octaves; ++i) {
    sum += sample(x * frequency, y * frequency) * amplitude;
    ampSum += amplitude;
    amplitude *= cfg.persistence;
    frequency *= cfg.lacunarity;
  }

  return (ampSum > 0.0f) ? (sum / ampSum) : 0.0f;
}

}  // namespace terrain

