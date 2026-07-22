#pragma once

// Author: Karl-Johan Bailey
//
// SkyComponent (descriptive only)
// Describes the sky covering the world (clouds, sun disc visuals, etc).
// This component does NOT represent a light; it is purely visual description.
//
// A rendering/sky system can interpret this component and configure the sky shader/material.

#include "ecs/EntityId.h"
#include "math/Vec3.h"
#include "math/Vec2.h"
#include "render/AssetRef.h"
#include "render/Color.h"

#include <cstdint>

namespace ecs {

struct SkyComponent {
  enum class Mode {
    Procedural,
    Skybox,  // cubemap/texture-based
  };

  enum class SkyType {
    Day,
    Sunset,
    Night,
    Overcast,
    Storm,
  };

  enum class CloudType {
    None,
    Wispy,
    Scattered,
    Broken,
    Overcast,
    Storm,
  };

  enum class Quality {
    Low,
    Medium,
    High,
    Ultra,
  };

  bool enabled = true;
  Mode mode = Mode::Procedural;
  SkyType skyType = SkyType::Day;
  CloudType cloudType = CloudType::Scattered;
  Quality quality = Quality::Medium;

  // If enabled, a system can treat `skyType` as a high-level preset and drive related visuals
  // (colors, sun disc, stars, etc).
  bool useSkyTypePreset = true;

  // Optional: link other environment components so presets can keep the scene coherent.
  // - linkedDirectionalLightEntity: updates a directional LightComponent to match sun/moon.
  // - linkedFogVolumeEntity: updates fog density/color to match the sky type.
  EntityId linkedDirectionalLightEntity = kInvalidEntityId;
  EntityId linkedFogVolumeEntity = kInvalidEntityId;

  // Optional material/shader preset reference used by the sky renderer.
  // Example key: "materials/sky/day"
  render::AssetRef material;

  // Base gradient (procedural).
  render::Color horizonColor{0.65f, 0.75f, 0.95f, 1.0f};
  render::Color zenithColor{0.12f, 0.22f, 0.45f, 1.0f};

  // Sun disc visuals (not a light).
  bool sunEnabled = true;
  math::Vec3 sunDirection{0.2f, 0.9f, 0.2f};  // world-space direction the sun appears in
  render::Color sunTint{1.0f, 0.95f, 0.85f, 1.0f};
  float sunDiscIntensity = 1.0f;
  float sunDiscSize = 1.0f;  // system-defined scalar

  // Clouds (procedural).
  bool cloudsEnabled = true;
  float cloudCoverage = 0.35f;    // 0..1
  float cloudDensity = 0.6f;      // 0..1
  float cloudSpeed = 0.02f;       // units/sec (system-defined)
  math::Vec2 cloudWindDirection{1.0f, 0.6f};  // XZ wind dir
  float cloudTimeScale = 1.0f;    // animation multiplier
  float cloudTurbulence = 0.35f;  // 0..1, affects warping/detail
  float cloudScale = 1.0f;        // tiling
  float cloudLightAbsorption = 0.4f;

  // Cloud layer height (world units, meters).
  float cloudHeightMeters = 150.0f;

  // Stars (procedural).
  bool starsEnabled = true;
  float starsIntensity = 0.75f;   // brightness multiplier
  float starsDensity = 0.55f;     // 0..1, how many stars
  float starsSize = 0.9f;         // 0..2-ish, bigger = fatter points
  float starsTwinkleStrength = 0.25f; // 0..1
  float starsTwinkleSpeed = 0.6f; // Hz-ish
  std::uint32_t starsSeed = 1337u;
};

}  // namespace ecs
