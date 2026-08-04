#pragma once

// Author: Karl-Johan Bailey
//
// Dirt material preset (high-quality PBR-style descriptor).

#include "ecs/components/ShaderComponent.h"
#include "render/Color.h"

namespace materials::presets {

inline ecs::ShaderComponent Dirt() {
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

  s.textures.push_back({"albedo", render::AssetRef{true, "textures/dirt_albedo", 0}, true});
  s.textures.push_back({"normal", render::AssetRef{true, "textures/dirt_normal", 0}, false});
  s.textures.push_back({"orm", render::AssetRef{true, "textures/dirt_orm", 0}, false});  // occlusion/rough/metal

  s.parameters.push_back({"baseColor", render::Color{0.65f, 0.55f, 0.45f, 1.0f}});
  s.parameters.push_back({"roughness", 0.95f});
  s.parameters.push_back({"metallic", 0.0f});
  s.parameters.push_back({"specularIntensity", 1.0f});
  s.parameters.push_back({"dirtColorNoiseStrength", 0.45f});
  s.parameters.push_back({"normalScale", 1.0f});
  s.parameters.push_back({"aoStrength", 1.0f});
  s.parameters.push_back({"uvTiling", math::Vec2{1.0f, 1.0f}});

  return s;
}

}  // namespace materials::presets
