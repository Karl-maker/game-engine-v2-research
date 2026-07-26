#pragma once

// Author: Karl-Johan Bailey
//
// FileChunkSource
// Loads chunk definitions from a JSON file.

#include "ecs/services/IChunkSource.h"

#include <string>
#include <unordered_map>
#include <filesystem>

namespace ecs::services {

class FileChunkSource final : public IChunkSource {
 public:
  explicit FileChunkSource(std::string path);

  bool reload();
  bool reloadIfChanged();
  const std::string& path() const { return m_path; }

  std::optional<ChunkDefinition> loadChunk(const ChunkCoord& coord) override;

 private:
  std::string m_path;
  std::unordered_map<ChunkCoord, ChunkDefinition, ChunkCoordHash> m_chunks;
  std::filesystem::file_time_type m_lastWriteTime{};
};

}  // namespace ecs::services
