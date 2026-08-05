#include "ecs/services/ChunkStreamingService.h"

// Author: Karl-Johan Bailey

#include "ecs/EntityRegistry.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

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
  const int maxLoadsPerTick = std::max(0, m_config.maxLoadsPerTick);
  const int maxUnloadsPerTick = std::max(0, m_config.maxUnloadsPerTick);

  // --- Load nearby chunks ---
  std::vector<std::pair<ChunkCoord, float>> loadCandidates;
  loadCandidates.reserve(static_cast<std::size_t>((searchR * 2 + 1) * (searchR * 2 + 1)));
  for (int dy = -searchR; dy <= searchR; ++dy) {
    for (int dx = -searchR; dx <= searchR; ++dx) {
      const ChunkCoord coord{center.x + dx, center.y + dy};
      const float dist = distanceToChunkAabbXZ(anchorTr->position, coord, m_config.chunkSizeMeters);
      if (dist > loadProx) continue;
      if (m_loaded.find(coord) != m_loaded.end()) continue;
      loadCandidates.emplace_back(coord, dist);
    }
  }

  std::sort(loadCandidates.begin(), loadCandidates.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

  int loadsThisTick = 0;
  for (const auto& [coord, _] : loadCandidates) {
    if (maxLoadsPerTick > 0 && loadsThisTick >= maxLoadsPerTick) break;

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
    ++loadsThisTick;
    std::cout << "[chunks] loaded (" << coord.x << "," << coord.y << ")\n";
  }

  // --- Unload far chunks ---
  std::vector<std::pair<ChunkCoord, float>> toUnload;
  toUnload.reserve(m_loaded.size());
  for (const auto& [coord, _] : m_loaded) {
    const float dist = distanceToChunkAabbXZ(anchorTr->position, coord, m_config.chunkSizeMeters);
    if (dist > unloadProx) {
      toUnload.emplace_back(coord, dist);
    }
  }

  std::sort(toUnload.begin(), toUnload.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

  int unloadsThisTick = 0;
  for (const auto& [coord, _] : toUnload) {
    if (maxUnloadsPerTick > 0 && unloadsThisTick >= maxUnloadsPerTick) break;
    auto it = m_loaded.find(coord);
    if (it == m_loaded.end()) continue;
    for (const EntityId id : it->second.entities) {
      if (registry.isAlive(id)) registry.destroyEntity(id);
    }
    m_loaded.erase(it);
    ++unloadsThisTick;
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
