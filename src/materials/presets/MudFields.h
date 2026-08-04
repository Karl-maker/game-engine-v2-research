#pragma once

// Author: Karl-Johan Bailey
//
// High-quality mud/field ground material preset using the provided mud texture set.

#include "ecs/components/ShaderComponent.h"
#include "math/Vec2.h"
#include "render/Color.h"

namespace materials::presets {

inline ecs::ShaderComponent MudFields() {
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

  // Mud texture set (OpenGL normal map).
  s.textures.push_back({"albedo", render::AssetRef{true, "assets/textures/mud/mat0_c.jpg", 0}, true});
  s.textures.push_back({"normalgl", render::AssetRef{true, "assets/textures/mud/mat0_n.jpg", 0}, false});
  s.textures.push_back({"roughness", render::AssetRef{true, "assets/textures/mud/mat0_r.jpg", 0}, false});
  s.textures.push_back({"ao", render::AssetRef{true, "assets/textures/mud/mat0_ao.png", 0}, false});
  s.textures.push_back({"displacement", render::AssetRef{true, "assets/textures/mud/mat0_g.jpg", 0}, false});

  // Mix in some dirt as a subtle secondary layer.
  s.textures.push_back({"rock_albedo", render::AssetRef{true, "assets/textures/dirt/dirt_color.jpg", 0}, true});
  s.textures.push_back({"rock_normalgl", render::AssetRef{true, "assets/textures/dirt/dirt_normalgl.jpg", 0}, false});
  s.textures.push_back({"rock_roughness", render::AssetRef{true, "assets/textures/dirt/dirt_roughness.jpg", 0}, false});
  s.textures.push_back({"rock_ao", render::AssetRef{true, "assets/textures/dirt/dirt_ambientocclusion.jpg", 0}, false});
  s.textures.push_back({"rock_displacement", render::AssetRef{true, "assets/textures/dirt/dirt_displacement.jpg", 0}, false});

  // Base params (the shader uses these even when textures are present).
  s.parameters.push_back({"baseColor", render::Color{0.22f, 0.20f, 0.17f, 1.0f}});
  s.parameters.push_back({"roughness", 1.0f});
  s.parameters.push_back({"roughnessInvert", true});
  s.parameters.push_back({"metallic", 0.0f});
  s.parameters.push_back({"specularIntensity", 0.22f});
  s.parameters.push_back({"dirtColorNoiseStrength", 0.18f});
  s.parameters.push_back({"uvTiling", math::Vec2{11.0f, 11.0f}});
  s.parameters.push_back({"normalScale", 1.35f});
  s.parameters.push_back({"aoStrength", 0.60f});
  s.parameters.push_back({"displacementStrength", 0.05f});
  s.parameters.push_back({"displacementInvert", true});

  // Higher-quality tessellation close to camera.
  s.parameters.push_back({"tessNear", 7.0f});
  s.parameters.push_back({"tessFar", 70.0f});
  s.parameters.push_back({"tessMin", 1.0f});
  s.parameters.push_back({"tessMax", 18.0f});
  s.parameters.push_back({"tessQuality", 1});  // High

  // Dirt blend layer controls (mapped onto the shader's "rock layer").
  s.parameters.push_back({"rockLayerEnabled", true});
  s.parameters.push_back({"rockUvTiling", math::Vec2{14.0f, 14.0f}});
  s.parameters.push_back({"rockNormalScale", 1.6f});
  s.parameters.push_back({"rockDisplacementStrength", 0.15f});
  s.parameters.push_back({"rockBlendStrength", 0.10f});
  s.parameters.push_back({"rockNoiseScale", 0.075f});

  return s;
}

}  // namespace materials::presets
