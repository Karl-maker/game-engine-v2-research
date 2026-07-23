#include "ecs/services/SpatialHashGridService.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <cmath>
#include <limits>

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

std::vector<ecs::EntityId> SpatialHashGridService::queryAabb(const Aabb& bounds) const {
  std::vector<ecs::EntityId> out;
  std::unordered_set<ecs::EntityId> seen;
  const CellCoord minC = coordFor(bounds.min);
  const CellCoord maxC = coordFor(bounds.max);
  for (int z = minC.z; z <= maxC.z; ++z) {
    for (int y = minC.y; y <= maxC.y; ++y) {
      for (int x = minC.x; x <= maxC.x; ++x) {
        const auto it = m_cells.find(keyFor({x, y, z}));
        if (it == m_cells.end()) continue;
        for (ecs::EntityId entity : it->second) {
          if (seen.insert(entity).second) {
            out.push_back(entity);
          }
        }
      }
    }
  }
  return out;
}

std::vector<ecs::EntityId> SpatialHashGridService::entitiesAlongRay(const math::Vec3& origin,
                                                                    const math::Vec3& direction,
                                                                    float length) const {
  std::vector<ecs::EntityId> out;
  if (length <= 0.0f || math::lengthSq(direction) <= 0.0f) {
    return out;
  }

  const math::Vec3 dir = math::normalize(direction);
  const float epsilon = 1e-6f;
  const float cell = m_cellSizeMeters;
  const float maxT = length;
  CellCoord cellCoord = coordFor(origin);

  const auto stepFor = [](float v) -> int { return (v > 0.0f) - (v < 0.0f); };
  const int stepX = stepFor(dir.x);
  const int stepY = stepFor(dir.y);
  const int stepZ = stepFor(dir.z);

  const auto boundaryT = [&](int coord, float o, float d, int step) -> float {
    if (std::fabs(d) < epsilon || step == 0) return std::numeric_limits<float>::infinity();
    const float nextBoundary = step > 0 ? (static_cast<float>(coord + 1) * cell) : (static_cast<float>(coord) * cell);
    return (nextBoundary - o) / d;
  };

  const auto deltaT = [&](float d) -> float {
    if (std::fabs(d) < epsilon) return std::numeric_limits<float>::infinity();
    return cell / std::fabs(d);
  };

  float tMaxX = boundaryT(cellCoord.x, origin.x, dir.x, stepX);
  float tMaxY = boundaryT(cellCoord.y, origin.y, dir.y, stepY);
  float tMaxZ = boundaryT(cellCoord.z, origin.z, dir.z, stepZ);
  const float tDeltaX = deltaT(dir.x);
  const float tDeltaY = deltaT(dir.y);
  const float tDeltaZ = deltaT(dir.z);

  std::unordered_set<ecs::EntityId> seen;
  auto appendCell = [&](const CellCoord& c) {
    const auto it = m_cells.find(keyFor(c));
    if (it == m_cells.end()) return;
    for (ecs::EntityId entity : it->second) {
      if (seen.insert(entity).second) {
        out.push_back(entity);
      }
    }
  };

  appendCell(cellCoord);
  float t = 0.0f;
  while (t <= maxT) {
    if (tMaxX < tMaxY) {
      if (tMaxX < tMaxZ) {
        cellCoord.x += stepX;
        t = tMaxX;
        tMaxX += tDeltaX;
      } else {
        cellCoord.z += stepZ;
        t = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    } else {
      if (tMaxY < tMaxZ) {
        cellCoord.y += stepY;
        t = tMaxY;
        tMaxY += tDeltaY;
      } else {
        cellCoord.z += stepZ;
        t = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    }
    if (t > maxT) break;
    appendCell(cellCoord);
  }

  return out;
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
