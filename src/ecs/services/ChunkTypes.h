#pragma once

// Author: Karl-Johan Bailey
//
// ChunkTypes
// Minimal types for chunk streaming + spawning.

#include "data/Json.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ecs::services {

struct ChunkCoord final {
  int x = 0;  // world +X direction
  int y = 0;  // world +Z direction

  friend bool operator==(const ChunkCoord& a, const ChunkCoord& b) { return a.x == b.x && a.y == b.y; }
  friend bool operator!=(const ChunkCoord& a, const ChunkCoord& b) { return !(a == b); }
};

struct ChunkCoordHash final {
  std::size_t operator()(const ChunkCoord& c) const noexcept {
    const std::uint64_t ux = static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.x));
    const std::uint64_t uy = static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.y));
    const std::uint64_t key = (ux << 32) ^ uy;
    // splitmix64-ish finalizer
    std::uint64_t z = key + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z = z ^ (z >> 31);
    return static_cast<std::size_t>(z);
  }
};

struct ChunkEntitySpawn final {
  std::string factory;
  data::JsonValue config;
};

struct ChunkDefinition final {
  ChunkCoord coord{};
  std::vector<ChunkEntitySpawn> entities;
};

}  // namespace ecs::services

