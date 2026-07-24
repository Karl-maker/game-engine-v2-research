#pragma once

// Author: Karl-Johan Bailey
//
// VfxFactory
// Creates a visual-effect emitter entity from data.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/components/VfxComponent.h"
#include "math/Vec3.h"

#include <string>

namespace ecs::services {

struct VfxConfig final {
  std::string name;
  math::Vec3 position{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationDeg{0.0f, 0.0f, 0.0f};
  math::Vec3 scale{1.0f, 1.0f, 1.0f};
  ecs::VfxComponent component{};
};

class VfxFactory final {
 public:
  EntityId create(EntityRegistry& registry, const VfxConfig& config);
};

}  // namespace ecs::services

