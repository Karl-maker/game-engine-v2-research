#pragma once

// Author: Karl-Johan Bailey
//
// MeshRendererComponent (descriptive only)
// Lightweight rendering hints for meshes (wireframe/opaque/etc).
// A renderer interprets these values.

#include "physics/LayerMask.h"

#include <cstdint>

namespace ecs {

struct MeshRendererComponent {
  enum class RenderMode {
    Opaque,
    Transparent,
    Wireframe,
  };

  bool enabled = true;
  RenderMode renderMode = RenderMode::Opaque;

  // Engine-defined asset handles/ids.
  bool hasMesh = false;
  std::uint32_t meshId = 0;

  bool hasMaterial = false;
  std::uint32_t materialId = 0;

  // Optional visibility/layer mask (renderer-defined).
  physics::LayerMask visibleLayers = physics::kAllLayers;
};

}  // namespace ecs

