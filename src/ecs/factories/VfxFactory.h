#pragma once

// Author: Karl-Johan Bailey
//
// VfxFactory
// Spawns a visual-effect emitter from config data.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/components/VfxComponent.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct VfxConfig final {
  TransformInput transform{.name = "vfx"};
  ecs::VfxComponent vfx{};
};

class VfxFactory final {
 public:
  EntityId create(EntityRegistry& registry, const VfxConfig& config);
};

}  // namespace ecs::services
