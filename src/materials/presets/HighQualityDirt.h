#pragma once

// Author: Karl-Johan Bailey
//
// High-quality dirt/ground material preset using the provided ground texture set.

#include "ecs/components/ShaderComponent.h"
#include "render/Color.h"
#include "math/Vec2.h"

namespace materials::presets {

inline ecs::ShaderComponent HighQualityDirt() {
  ecs::ShaderComponent s;
  s.shader.key = "shaders/pbr";
  s.renderMode = render::RenderMode::Opaque;
  s.cullMode = render::CullMode::Back;
  s.depthTest = render::DepthTest::LessEqual;
  s.depthWrite = true;
  s.blendMode = render::BlendMode::Disabled;
  s.doubleSided = false;
  s.receiveShadows = true;
  s.castShadows = true;

  // Dirt texture set (OpenGL normal map).
  s.textures.push_back({"albedo", render::AssetRef{true, "assets/textures/dirt/dirt_color.jpg", 0}, true});
  s.textures.push_back({"normalgl", render::AssetRef{true, "assets/textures/dirt/dirt_normalgl.jpg", 0}, false});
  s.textures.push_back({"roughness", render::AssetRef{true, "assets/textures/dirt/dirt_roughness.jpg", 0}, false});
  s.textures.push_back(
      {"ao", render::AssetRef{true, "assets/textures/dirt/dirt_ambientocclusion.jpg", 0}, false});
  s.textures.push_back(
      {"displacement", render::AssetRef{true, "assets/textures/dirt/dirt_displacement.jpg", 0}, false});

  // Base params (the shader uses these even when textures are present).
  s.parameters.push_back({"baseColor", render::Color{0.65f, 0.55f, 0.45f, 1.0f}});
  s.parameters.push_back({"roughness", 1.0f});
  s.parameters.push_back({"metallic", 0.0f});
  s.parameters.push_back({"specularIntensity", 0.55f});
  s.parameters.push_back({"dirtColorNoiseStrength", 0.25f});
  s.parameters.push_back({"uvTiling", math::Vec2{14.0f, 14.0f}});
  // Amplify normal map detail a bit.
  s.parameters.push_back({"normalScale", 1.8f});
  // Lift AO so shadows aren't crushed.
  s.parameters.push_back({"aoStrength", 0.45f});
  // Base displacement influence (shading/bump only).
  s.parameters.push_back({"displacementStrength", 0.82f});

  return s;
}

}  // namespace materials::presets
