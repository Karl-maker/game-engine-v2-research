#pragma once

// Author: Karl-Johan Bailey
//
// Pebbly dirt preset
// Adds a procedural pebble layer on top of the Dirt preset.

#include "materials/presets/Dirt.h"

namespace materials::presets {

inline ecs::ShaderComponent PebblyDirt() {
  ecs::ShaderComponent s = Dirt();

  // Enable the pebble layer in the OpenGL demo shader.
  s.parameters.push_back({"pebblesEnabled", true});
  // Keep close to dirt (slightly warmer/lighter).
  s.parameters.push_back({"pebbleColor", render::Color{0.58f, 0.52f, 0.44f, 1.0f}});
  s.parameters.push_back({"pebbleRoughness", 0.7f});
  // Bigger scale => smaller pebbles (more cells per meter).
  s.parameters.push_back({"pebbleScale", 3.25f});
  s.parameters.push_back({"pebbleDensity", 0.32f});
  s.parameters.push_back({"pebbleBlend", 0.55f});
  s.parameters.push_back({"pebbleNormalStrength", 0.95f});
  s.parameters.push_back({"pebbleHeight", 0.85f});

  // Keep sinks subtle when pebbles are enabled.
  s.parameters.push_back({"dirtSinksEnabled", true});
  s.parameters.push_back({"dirtSinkStrength", 0.07f});
  s.parameters.push_back({"dirtSinkScale", 1.75f});
  s.parameters.push_back({"dirtSinkDensity", 0.25f});

  return s;
}

}  // namespace materials::presets
