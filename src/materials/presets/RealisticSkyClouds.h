#pragma once

// Author: Karl-Johan Bailey
//
// RealisticSkyClouds (demo)
// Pairs with `graphics/shaders/sky.*.glsl` and uses SkyComponent for high-level settings.

#include "ecs/components/ShaderComponent.h"

namespace materials::presets {

inline ecs::ShaderComponent RealisticSkyClouds() {
  ecs::ShaderComponent s;
  s.shader.key = "graphics/shaders/sky";
  s.renderMode = render::RenderMode::Opaque;
  s.cullMode = render::CullMode::None;
  s.depthTest = render::DepthTest::Disabled;
  s.depthWrite = false;
  s.blendMode = render::BlendMode::Disabled;
  s.doubleSided = true;
  s.receiveShadows = false;
  s.castShadows = false;
  return s;
}

}  // namespace materials::presets
