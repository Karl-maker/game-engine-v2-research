#pragma once

// Author: Karl-Johan Bailey
//
// Reusable collision/layer bitmask.

#include <cstdint>

namespace physics {

using LayerMask = std::uint32_t;
static constexpr LayerMask kAllLayers = 0xFFFFFFFFu;

}  // namespace physics

