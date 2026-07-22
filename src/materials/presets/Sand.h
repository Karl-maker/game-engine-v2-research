#pragma once

// Author: Karl-Johan Bailey
//
// Sand material preset (high-quality PBR-style descriptor).

#include "ecs/components/ShaderComponent.h"
#include "render/Color.h"

namespace materials::presets {

inline ecs::ShaderComponent Sand() {
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

  s.textures.push_back({"albedo", render::AssetRef{true, "textures/sand_albedo", 0}, true});
  s.textures.push_back({"normal", render::AssetRef{true, "textures/sand_normal", 0}, false});
  s.textures.push_back({"orm", render::AssetRef{true, "textures/sand_orm", 0}, false});

  s.parameters.push_back({"baseColor", render::Color{0.85f, 0.78f, 0.62f, 1.0f}});
  s.parameters.push_back({"roughness", 0.9f});
  s.parameters.push_back({"metallic", 0.0f});
  s.parameters.push_back({"normalScale", 0.8f});
  s.parameters.push_back({"uvTiling", math::Vec2{2.0f, 2.0f}});

  return s;
}

}  // namespace materials::presets

