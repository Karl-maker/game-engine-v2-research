#pragma once

// Author: Karl-Johan Bailey
//
// VfxComponent (descriptive only)
// Describes a visual effect emitter such as fire, electricity, sparks, smoke, or steam.
// Rendering systems interpret these settings and apply LOD / quality-based budgets.

#include "render/Color.h"

#include <cstdint>

namespace ecs {

struct VfxComponent final {
  enum class Type {
    Fire,
    Electricity,
    Sparks,
    Smoke,
    Steam,
  };

  enum class Quality {
    Low,
    Medium,
    High,
    Ultra,
  };

  bool enabled = true;
  Type type = Type::Fire;
  Quality quality = Quality::High;
  bool autoQuality = true;

  // LOD / culling controls.
  float maxRenderDistance = 96.0f;
  float lodNearDistance = 16.0f;
  float lodMidDistance = 32.0f;
  float lodFarDistance = 56.0f;
  float lodUltraDistance = 80.0f;
  float lodForceNearDistance = 8.0f;
  float viewDotBias = 0.0f;

  // Shared emitter settings.
  float intensity = 1.0f;
  float spawnRate = 18.0f;
  float burstInterval = 0.0f;
  float lifetimeSeconds = 1.2f;
  float sizeMeters = 0.35f;
  float sizeVariance = 0.35f;
  float speedMetersPerSecond = 1.4f;
  float speedVariance = 0.45f;
  float gravityScale = 0.35f;
  float drag = 0.08f;
  float flickerStrength = 0.35f;
  float flickerSpeed = 8.0f;
  bool looping = true;
  bool castLight = false;
  std::uint32_t seed = 1337u;

  // Fire / smoke shaping.
  float heightMeters = 1.25f;
  float upwardBias = 0.65f;
  float spreadRadiusMeters = 0.45f;
  float heatHazeStrength = 0.25f;

  // Electricity shaping.
  float chargeLengthMeters = 3.5f;
  float arcJitter = 0.45f;
  int branchCount = 4;
  int segmentCount = 8;
  float pulseSpeed = 12.0f;

  // Sparks shaping.
  int sparkCount = 16;
  float sparkSpreadDegrees = 28.0f;
  float sparkTrailLengthMeters = 0.6f;
  float sparkFadeSeconds = 0.25f;

  render::Color primaryColor{1.0f, 0.55f, 0.10f, 1.0f};
  render::Color secondaryColor{1.0f, 0.95f, 0.35f, 1.0f};
};

}  // namespace ecs

