#pragma once

// Author: Karl-Johan Bailey
//
// SkyPresetSystem
// Applies high-level sky presets (SkyComponent::skyType) onto related component values.
//
// This keeps "day/night" style coherent by updating:
// - SkyComponent colors/sun/stars
// - (Optionally) a linked directional LightComponent
// - (Optionally) a linked FogVolumeComponent

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class SkyPresetSystem final {
 public:
  void tick(ecs::EntityRegistry& registry) const;
};

}  // namespace ecs::systems

