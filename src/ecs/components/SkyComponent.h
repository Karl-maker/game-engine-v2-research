#pragma once

// Author: Karl-Johan Bailey
//
// SkyComponent (descriptive only)
// Describes the sky covering the world (clouds, sun disc visuals, etc).
// This component does NOT represent a light; it is purely visual description.
//
// A rendering/sky system can interpret this component and configure the sky shader/material.

#include "math/Vec3.h"
#include "render/AssetRef.h"
#include "render/Color.h"

namespace ecs {

struct SkyComponent {
  enum class Mode {
    Procedural,
    Skybox,  // cubemap/texture-based
  };

  bool enabled = true;
  Mode mode = Mode::Procedural;

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
  float cloudScale = 1.0f;        // tiling
  float cloudLightAbsorption = 0.4f;
};

}  // namespace ecs

