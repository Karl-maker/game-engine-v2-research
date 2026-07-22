#pragma once

// Author: Karl-Johan Bailey
//
// Mulch material preset (high-quality PBR-style descriptor).

#include "ecs/components/ShaderComponent.h"
#include "render/Color.h"

namespace materials::presets {

inline ecs::ShaderComponent Mulch() {
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

  s.textures.push_back({"albedo", render::AssetRef{true, "textures/mulch_albedo", 0}, true});
  s.textures.push_back({"normal", render::AssetRef{true, "textures/mulch_normal", 0}, false});
  s.textures.push_back({"orm", render::AssetRef{true, "textures/mulch_orm", 0}, false});
  s.textures.push_back({"height", render::AssetRef{true, "textures/mulch_height", 0}, false});

  s.parameters.push_back({"baseColor", render::Color{0.25f, 0.18f, 0.12f, 1.0f}});
  s.parameters.push_back({"roughness", 0.98f});
  s.parameters.push_back({"metallic", 0.0f});
  s.parameters.push_back({"normalScale", 1.1f});
  s.parameters.push_back({"parallaxScale", 0.02f});  // if shader supports
  s.parameters.push_back({"uvTiling", math::Vec2{1.5f, 1.5f}});

  return s;
}

}  // namespace materials::presets

