#pragma once

// Author: Karl-Johan Bailey
//
// AssetRef (descriptive)
// Lightweight reference to an asset (shader, texture, etc).
// Asset loading/lookup is engine-defined; these fields are intentionally generic.

#include <cstdint>
#include <string>

namespace render {

struct AssetRef {
  bool enabled = true;

  // Prefer using a stable key/path (e.g. "shaders/pbr", "textures/dirt_albedo").
  std::string key;

  // Optional numeric handle/id assigned by an asset manager.
  std::uint32_t id = 0;
};

}  // namespace render

