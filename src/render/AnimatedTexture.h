#pragma once

// Author: Karl-Johan Bailey
//
// AnimatedTexture (descriptive)
// Describes video-style playback from a sequence of image frames.

#include "render/AssetRef.h"

#include <vector>

namespace render {

struct AnimatedTexture final {
  bool enabled = false;
  std::vector<AssetRef> frames;
  float framesPerSecond = 12.0f;
  bool looping = true;
  bool pingPong = false;
  bool holdLastFrame = true;
  int startFrame = 0;
};

}  // namespace render
