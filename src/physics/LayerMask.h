#pragma once

// Author: Karl-Johan Bailey
//
// Reusable collision/layer bitmask.

#include <cstdint>

namespace physics {

using LayerMask = std::uint32_t;
static constexpr LayerMask kLayerWorld = 1u << 0;
static constexpr LayerMask kLayerCharacter = 1u << 1;
static constexpr LayerMask kAllLayers = 0xFFFFFFFFu;

}  // namespace physics
