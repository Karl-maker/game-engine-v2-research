#pragma once

// Author: Karl-Johan Bailey
//
// High-quality dirt with a noisy rock layer + grass ground layer blended in.

#include "materials/presets/HighQualityDirtRockLayer.h"

namespace materials::presets {

inline ecs::ShaderComponent HighQualityDirtRockGrassLayer() {
  ecs::ShaderComponent s = HighQualityDirtRockLayer();

  // Grass layer textures.
  s.textures.push_back({"grass_albedo", render::AssetRef{true, "assets/textures/grass/grass_color.jpg", 0}, true});
  s.textures.push_back({"grass_normalgl", render::AssetRef{true, "assets/textures/grass/grass_normalgl.jpg", 0}, false});
  s.textures.push_back({"grass_roughness", render::AssetRef{true, "assets/textures/grass/grass_roughness.jpg", 0}, false});
  s.textures.push_back({"grass_ao", render::AssetRef{true, "assets/textures/grass/grass_ambientocclusion.jpg", 0}, false});
  s.textures.push_back({"grass_displacement", render::AssetRef{true, "assets/textures/grass/grass_displacement.jpg", 0}, false});

  // Grass blending: mostly on flatter areas + noisy patches.
  s.parameters.push_back({"grassLayerEnabled", true});
  s.parameters.push_back({"grassUvTiling", math::Vec2{22.0f, 22.0f}});
  s.parameters.push_back({"grassNormalScale", 1.6f});
  s.parameters.push_back({"grassDisplacementStrength", 0.35f});
  s.parameters.push_back({"grassBlendStrength", 1.15f});
  s.parameters.push_back({"grassNoiseScale", 0.07f});
  s.parameters.push_back({"grassSlopeBias", 0.35f});  // higher => less grass on slopes

  return s;
}

}  // namespace materials::presets
