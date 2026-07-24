#include "ecs/services/ChunkStreamingService.h"

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace ecs::services {

namespace {

ChunkCoord worldToChunk(const math::Vec3& worldPos, float chunkSizeMeters) {
  const float s = std::max(0.01f, chunkSizeMeters);
  const int cx = static_cast<int>(std::floor((worldPos.x + 0.5f * s) / s));
  const int cy = static_cast<int>(std::floor((worldPos.z + 0.5f * s) / s));
  return {cx, cy};
}

math::Vec3 chunkOriginWorld(const ChunkCoord& c, float chunkSizeMeters) {
  const float s = std::max(0.01f, chunkSizeMeters);
  return {static_cast<float>(c.x) * s, 0.0f, static_cast<float>(c.y) * s};
}

float distanceToChunkAabbXZ(const math::Vec3& worldPos, const ChunkCoord& coord, float chunkSizeMeters) {
  const float s = std::max(0.01f, chunkSizeMeters);
  const float cx = static_cast<float>(coord.x) * s;
  const float cz = static_cast<float>(coord.y) * s;
  const float half = 0.5f * s;
  const float minX = cx - half;
  const float minZ = cz - half;
  const float maxX = cx + half;
  const float maxZ = cz + half;

  const float px = worldPos.x;
  const float pz = worldPos.z;

  float dx = 0.0f;
  if (px < minX) dx = minX - px;
  else if (px > maxX) dx = px - maxX;

  float dz = 0.0f;
  if (pz < minZ) dz = minZ - pz;
  else if (pz > maxZ) dz = pz - maxZ;

  return std::sqrt(dx * dx + dz * dz);
}

}  // namespace

void ChunkStreamingService::tick(EntityRegistry& registry,
                                EntityId anchorEntity,
                                IChunkSource& source,
                                const EntityFactoryRegistry& factories) {
  if (anchorEntity == kInvalidEntityId) return;
  const auto* anchorTr = registry.tryGet<ecs::TransformComponent>(anchorEntity);
  if (!anchorTr) return;

  const ChunkCoord center = worldToChunk(anchorTr->position, m_config.chunkSizeMeters);
  const int searchR = std::max(0, m_config.searchRadiusChunks);
  const float loadProx = std::max(0.0f, m_config.loadProximityMeters);
  const float unloadProx = std::max(loadProx, m_config.unloadProximityMeters);

  // --- Load nearby chunks ---
  for (int dy = -searchR; dy <= searchR; ++dy) {
    for (int dx = -searchR; dx <= searchR; ++dx) {
      const ChunkCoord coord{center.x + dx, center.y + dy};
      const float dist = distanceToChunkAabbXZ(anchorTr->position, coord, m_config.chunkSizeMeters);
      if (dist > loadProx) continue;
      if (m_loaded.find(coord) != m_loaded.end()) continue;

      auto defOpt = source.loadChunk(coord);
      if (!defOpt.has_value()) continue;

      LoadedChunk loaded{};
      loaded.entities.reserve(defOpt->entities.size());

      FactoryContext fctx{};
      fctx.chunkOriginWorld = chunkOriginWorld(coord, m_config.chunkSizeMeters);

      for (const auto& spawn : defOpt->entities) {
        IEntityFactory* f = factories.find(spawn.factory);
        if (!f) {
          std::cerr << "[chunks] missing factory \"" << spawn.factory << "\" for chunk (" << coord.x << "," << coord.y << ")\n";
          continue;
        }
        const EntityId created = f->create(registry, spawn.config, fctx);
        if (created != kInvalidEntityId) {
          loaded.entities.push_back(created);
        }
      }

      m_loaded.emplace(coord, std::move(loaded));
      std::cout << "[chunks] loaded (" << coord.x << "," << coord.y << ")\n";
    }
  }

  // --- Unload far chunks ---
  std::vector<ChunkCoord> toUnload;
  toUnload.reserve(m_loaded.size());
  for (const auto& [coord, _] : m_loaded) {
    const float dist = distanceToChunkAabbXZ(anchorTr->position, coord, m_config.chunkSizeMeters);
    if (dist > unloadProx) {
      toUnload.push_back(coord);
    }
  }

  for (const auto& coord : toUnload) {
    auto it = m_loaded.find(coord);
    if (it == m_loaded.end()) continue;
    for (const EntityId id : it->second.entities) {
      if (registry.isAlive(id)) registry.destroyEntity(id);
    }
    m_loaded.erase(it);
    std::cout << "[chunks] unloaded (" << coord.x << "," << coord.y << ")\n";
  }
}

void ChunkStreamingService::unloadAll(EntityRegistry& registry) {
  for (auto& [_, chunk] : m_loaded) {
    for (const EntityId id : chunk.entities) {
      if (registry.isAlive(id)) registry.destroyEntity(id);
    }
  }
  m_loaded.clear();
}

}  // namespace ecs::services
