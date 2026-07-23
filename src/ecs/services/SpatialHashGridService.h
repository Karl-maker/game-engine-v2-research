#pragma once

// Author: Karl-Johan Bailey
//
// Uniform spatial hash broadphase for collision systems.
// It keeps pair generation near O(n + local pairs) instead of O(n^2).

#include "ecs/EntityId.h"
#include "math/Vec3.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ecs::services {

class SpatialHashGridService final {
 public:
  struct Aabb final {
    math::Vec3 min{};
    math::Vec3 max{};
  };

  struct Proxy final {
    ecs::EntityId entity = ecs::kInvalidEntityId;
    Aabb bounds{};
  };

  explicit SpatialHashGridService(float cellSizeMeters = 4.0f);

  void clear();
  void setCellSize(float cellSizeMeters);
  void insert(const Proxy& proxy);

  std::vector<std::pair<ecs::EntityId, ecs::EntityId>> candidatePairs() const;

 private:
  struct CellCoord final {
    int x = 0;
    int y = 0;
    int z = 0;
  };

  static std::int64_t keyFor(CellCoord c);
  CellCoord coordFor(const math::Vec3& p) const;

  float m_cellSizeMeters = 4.0f;
  std::unordered_map<std::int64_t, std::vector<ecs::EntityId>> m_cells;
};

}  // namespace ecs::services
