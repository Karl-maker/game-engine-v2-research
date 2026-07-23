#include "ecs/services/SpatialHashGridService.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <cmath>

namespace ecs::services {

SpatialHashGridService::SpatialHashGridService(float cellSizeMeters) { setCellSize(cellSizeMeters); }

void SpatialHashGridService::clear() { m_cells.clear(); }

void SpatialHashGridService::setCellSize(float cellSizeMeters) { m_cellSizeMeters = std::max(0.25f, cellSizeMeters); }

std::int64_t SpatialHashGridService::keyFor(CellCoord c) {
  const std::int64_t x = static_cast<std::int64_t>(c.x) & 0x1FFFFF;
  const std::int64_t y = static_cast<std::int64_t>(c.y) & 0x1FFFFF;
  const std::int64_t z = static_cast<std::int64_t>(c.z) & 0x1FFFFF;
  return (x << 42) ^ (y << 21) ^ z;
}

SpatialHashGridService::CellCoord SpatialHashGridService::coordFor(const math::Vec3& p) const {
  const float inv = 1.0f / m_cellSizeMeters;
  return {static_cast<int>(std::floor(p.x * inv)),
          static_cast<int>(std::floor(p.y * inv)),
          static_cast<int>(std::floor(p.z * inv))};
}

void SpatialHashGridService::insert(const Proxy& proxy) {
  const CellCoord minC = coordFor(proxy.bounds.min);
  const CellCoord maxC = coordFor(proxy.bounds.max);
  for (int z = minC.z; z <= maxC.z; ++z) {
    for (int y = minC.y; y <= maxC.y; ++y) {
      for (int x = minC.x; x <= maxC.x; ++x) {
        m_cells[keyFor({x, y, z})].push_back(proxy.entity);
      }
    }
  }
}

std::vector<std::pair<ecs::EntityId, ecs::EntityId>> SpatialHashGridService::candidatePairs() const {
  std::vector<std::pair<ecs::EntityId, ecs::EntityId>> out;
  std::unordered_set<std::uint64_t> seen;

  for (const auto& [_, entities] : m_cells) {
    for (std::size_t i = 0; i < entities.size(); ++i) {
      for (std::size_t j = i + 1; j < entities.size(); ++j) {
        ecs::EntityId a = entities[i];
        ecs::EntityId b = entities[j];
        if (a == b) continue;
        if (a > b) std::swap(a, b);
        const std::uint64_t key = (static_cast<std::uint64_t>(a) << 32) | static_cast<std::uint64_t>(b);
        if (seen.insert(key).second) out.emplace_back(a, b);
      }
    }
  }

  return out;
}

}  // namespace ecs::services
