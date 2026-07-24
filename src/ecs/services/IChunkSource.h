#pragma once

// Author: Karl-Johan Bailey
//
// IChunkSource
// Pluggable data provider for chunk definitions.

#include "ecs/services/ChunkTypes.h"

#include <optional>

namespace ecs::services {

class IChunkSource {
 public:
  virtual ~IChunkSource() = default;
  virtual std::optional<ChunkDefinition> loadChunk(const ChunkCoord& coord) = 0;
};

}  // namespace ecs::services

