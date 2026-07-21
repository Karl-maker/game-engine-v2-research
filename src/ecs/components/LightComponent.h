#pragma once

// Author: Karl-Johan Bailey
//
// LightComponent (descriptive only)
// Stores lighting parameters. Rendering systems interpret these values.

#include "math/Vec3.h"

#include <cstdint>

namespace ecs {

struct LightComponent {
  enum class Type {
    Directional,
    Point,
    Spot,
    Area,  // optional/engine-dependent
  };

  bool enabled = true;
  Type type = Type::Point;

  // Color in linear space (RGB). (1,1,1) is white.
  math::Vec3 color{1.0f, 1.0f, 1.0f};

  float intensity = 1.0f;

  bool hasTemperature = false;
  float temperatureKelvin = 6500.0f;

  float range = 10.0f;
  float radius = 0.0f;

  // Directional/spot direction (world-space; renderer/system decides).
  math::Vec3 direction{0.0f, -1.0f, 0.0f};

  // Spot angles in degrees.
  float spotInnerAngleDeg = 20.0f;
  float spotOuterAngleDeg = 30.0f;

  bool castShadows = false;
  std::uint32_t shadowResolution = 1024;
  float shadowBias = 0.001f;
  float normalBias = 0.0f;
  float shadowDistance = 50.0f;

  bool volumetricLighting = false;
  float volumetricIntensity = 1.0f;

  // Bitmasks/layers (engine-defined).
  std::uint32_t affectsLayers = 0xFFFFFFFFu;
  std::uint32_t lightMask = 0xFFFFFFFFu;

  float indirectMultiplier = 1.0f;
  float specularIntensity = 1.0f;
  float diffuseIntensity = 1.0f;

  // Optional assets (engine-defined). Kept as ids/handles later; string for now.
  bool hasCookieTexture = false;
  std::uint32_t cookieTextureId = 0;

  bool hasLensFlare = false;
  std::uint32_t lensFlareId = 0;
};

}  // namespace ecs

