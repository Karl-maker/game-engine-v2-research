#pragma once

// Author: Karl-Johan Bailey
//
// High-quality dirt/ground material preset using the provided ground texture set.

#include "ecs/components/ShaderComponent.h"
#include "render/Color.h"
#include "math/Vec2.h"

namespace materials::presets {

inline ecs::ShaderComponent Stone() {
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
  s.textures.push_back({"albedo", render::AssetRef{true, "assets/textures/grass/grass_color.jpg", 0}, true});
  s.textures.push_back({"normalgl", render::AssetRef{true, "assets/textures/grass/grass_normalgl.jpg", 0}, false});
  s.textures.push_back({"roughness", render::AssetRef{true, "assets/textures/grass/grass_roughness.jpg", 0}, false});
  s.textures.push_back(
      {"ao", render::AssetRef{true, "assets/textures/grass/grass_ambientocclusion.jpg", 0}, false});
  s.textures.push_back(
      {"displacement", render::AssetRef{true, "assets/textures/grass/grass_displacement.jpg", 0}, false});

  s.parameters.push_back({"roughness", 1.0f});
  s.parameters.push_back({"metallic", 0.0f});
  s.parameters.push_back({"specularIntensity", 0.55f});
  s.parameters.push_back({"dirtColorNoiseStrength", 0.05f});
  s.parameters.push_back({"uvTiling", math::Vec2{14.0f, 14.0f}});
  // Amplify normal map detail a bit.
  s.parameters.push_back({"normalScale", 1.8f});
  // Lift AO so shadows aren't crushed.
  s.parameters.push_back({"aoStrength", 0.45f});
  // Base displacement influence (shading/bump only).
  s.parameters.push_back({"displacementStrength", 0.42f});

  // Keep sinks subtle with textures.
  s.parameters.push_back({"dirtSinksEnabled", true});
  s.parameters.push_back({"dirtSinkStrength", 0.06f});
  s.parameters.push_back({"dirtSinkScale", 1.4f});
  s.parameters.push_back({"dirtSinkDensity", 0.25f});

  // Disable pebbles by default for this preset (textures provide detail).
  s.parameters.push_back({"pebblesEnabled", false});

  return s;
}

}  // namespace materials::presets
