#pragma once

// Author: Karl-Johan Bailey
//
// TextureBinding (descriptive)
// Binds a named texture slot to an asset reference.

#include "render/AssetRef.h"

#include <string>

namespace render {

struct TextureBinding {
  std::string slot;   // e.g. "albedo", "normal", "orm"
  AssetRef texture;   // asset ref for the texture
  bool srgb = true;   // typical for albedo; normal/orm are usually false
};

}  // namespace render

