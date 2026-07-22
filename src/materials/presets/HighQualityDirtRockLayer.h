#pragma once

// Author: Karl-Johan Bailey
//
// High-quality dirt with a noisy rock layer blended on top (no blocky tiles).

#include "materials/presets/HighQualityDirt.h"

namespace materials::presets {

inline ecs::ShaderComponent HighQualityDirtRockLayer() {
  ecs::ShaderComponent s = HighQualityDirt();

  // Pull back displacement a bit (tessellation + normal-derived micro-height adds extra lift).
  s.parameters.push_back({"displacementStrength", 0.10f});

  // Pull back tessellation quality for this material (still smooth near camera).
  s.parameters.push_back({"tessNear", 6.0f});
  s.parameters.push_back({"tessFar", 40.0f});
  s.parameters.push_back({"tessMin", 1.0f});
  s.parameters.push_back({"tessMax", 6.0f});
  s.parameters.push_back({"tessQuality", 0});  // Low

  // Rock layer textures.
  s.textures.push_back({"rock_albedo", render::AssetRef{true, "assets/textures/rock/rock_color.jpg", 0}, true});
  s.textures.push_back({"rock_normalgl", render::AssetRef{true, "assets/textures/rock/rock_normalgl.jpg", 0}, false});
  s.textures.push_back({"rock_roughness", render::AssetRef{true, "assets/textures/rock/rock_roughness.jpg", 0}, false});
  s.textures.push_back({"rock_ao", render::AssetRef{true, "assets/textures/rock/rock_ambientocclusion.jpg", 0}, false});
  s.textures.push_back(
      {"rock_displacement", render::AssetRef{true, "assets/textures/rock/rock_displacement.jpg", 0}, false});

  s.parameters.push_back({"rockLayerEnabled", true});
  s.parameters.push_back({"rockUvTiling", math::Vec2{18.0f, 18.0f}});
  s.parameters.push_back({"rockNormalScale", 2.0f});
  s.parameters.push_back({"rockDisplacementStrength", 0.10f});
  s.parameters.push_back({"rockBlendStrength", 0.7f});
  s.parameters.push_back({"rockNoiseScale", 0.05f});

  // Use true 3D rock instances instead of the old pebble shader layer.
  s.parameters.push_back({"pebblesEnabled", false});

  return s;
}

}  // namespace materials::presets
