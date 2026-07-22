#pragma once

// Author: Karl-Johan Bailey
//
// Sky material preset (high-quality sky shader descriptor).
// Intended to be used by a sky renderer (not a mesh renderer).

#include "ecs/components/ShaderComponent.h"

namespace materials::presets {

inline ecs::ShaderComponent SkyDay() {
  ecs::ShaderComponent s;
  s.shader.key = "shaders/sky";
  s.renderMode = render::RenderMode::Opaque;
  s.cullMode = render::CullMode::None;
  s.depthTest = render::DepthTest::LessEqual;
  s.depthWrite = false;
  s.blendMode = render::BlendMode::Disabled;
  s.doubleSided = true;
  s.receiveShadows = false;
  s.castShadows = false;

  // Parameter names are shader-defined; these are common sky controls.
  s.parameters.push_back({"horizonColor", render::Color{0.65f, 0.75f, 0.95f, 1.0f}});
  s.parameters.push_back({"zenithColor", render::Color{0.12f, 0.22f, 0.45f, 1.0f}});

  s.parameters.push_back({"sunDirection", math::Vec3{0.2f, 0.9f, 0.2f}});
  s.parameters.push_back({"sunTint", render::Color{1.0f, 0.95f, 0.85f, 1.0f}});
  s.parameters.push_back({"sunDiscIntensity", 1.0f});
  s.parameters.push_back({"sunDiscSize", 1.0f});

  s.parameters.push_back({"cloudsEnabled", true});
  s.parameters.push_back({"cloudCoverage", 0.35f});
  s.parameters.push_back({"cloudDensity", 0.6f});
  s.parameters.push_back({"cloudSpeed", 0.02f});
  s.parameters.push_back({"cloudScale", 1.0f});
  s.parameters.push_back({"cloudLightAbsorption", 0.4f});

  return s;
}

}  // namespace materials::presets

