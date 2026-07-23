#pragma once

// Author: Karl-Johan Bailey
//
// HitDetectionSystem
// Calculates combat volume overlaps and emits combat contact events.

#include "ecs/EntityRegistry.h"
#include "ecs/services/EventService.h"
#include "ecs/services/SpatialHashGridService.h"

#include <cstdint>
#include <unordered_set>

namespace ecs::systems {

class HitDetectionSystem final {
 public:
  void tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds);

 private:
  ecs::services::SpatialHashGridService m_grid;
  std::unordered_set<std::uint64_t> m_activeContacts;
};

}  // namespace ecs::systems
