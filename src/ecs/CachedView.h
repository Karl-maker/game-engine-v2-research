#pragma once

// Author: Karl-Johan Bailey
//
// CachedView (utility)
// Caches the set of entities that match a component query based on
// EntityRegistry::structuralVersion() (component/entity presence changes).

#include "ecs/EntityRegistry.h"

#include <cstdint>
#include <vector>

namespace ecs {

template <typename... Components>
struct CachedView final {
  std::uint64_t version = 0;
  std::vector<EntityId> entities;

  void refresh(EntityRegistry& registry) {
    const std::uint64_t v = registry.structuralVersion();
    if (v == version) return;
    registry.collectEntities<Components...>(entities);
    version = v;
  }
};

}  // namespace ecs

