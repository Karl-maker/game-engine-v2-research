#pragma once

// Author: Karl-Johan Bailey
//
// ChunkStreamingService
// Loads/unloads chunk-defined entities around an anchor entity based on world position.

#include "ecs/EntityId.h"
#include "ecs/services/ChunkTypes.h"
#include "ecs/services/EntityFactoryRegistry.h"
#include "ecs/services/IChunkSource.h"

#include <unordered_map>
#include <vector>

namespace ecs {
class EntityRegistry;
}

namespace ecs::services {

struct ChunkStreamingConfig final {
  float chunkSizeMeters = 32.0f;
  int searchRadiusChunks = 4;  // only consider coords within this radius (square)
  float loadProximityMeters = 20.0f;
  float unloadProximityMeters = 28.0f;
};

class ChunkStreamingService final {
 public:
  explicit ChunkStreamingService(ChunkStreamingConfig config = {}) : m_config(config) {}

  void setConfig(ChunkStreamingConfig cfg) { m_config = cfg; }
  const ChunkStreamingConfig& config() const { return m_config; }

  void tick(EntityRegistry& registry,
            EntityId anchorEntity,
            IChunkSource& source,
            const EntityFactoryRegistry& factories);

  void unloadAll(EntityRegistry& registry);

 private:
  struct LoadedChunk final {
    std::vector<EntityId> entities;
  };

  ChunkStreamingConfig m_config{};
  std::unordered_map<ChunkCoord, LoadedChunk, ChunkCoordHash> m_loaded;
};

}  // namespace ecs::services
