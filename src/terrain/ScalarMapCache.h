#pragma once

// Author: Karl-Johan Bailey
//
// ScalarMapCache
// Loads grayscale image maps (height/surface masks) and provides fast sampling with mip support.

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace terrain {

class ScalarMapCache final {
 public:
  struct Map final {
    int width = 0;
    int height = 0;
    // Mip 0 is full resolution. Each mip is width*height bytes, 0..255.
    std::vector<std::vector<std::uint8_t>> mips;
  };

  ScalarMapCache() = default;
  ~ScalarMapCache() = default;

  ScalarMapCache(const ScalarMapCache&) = delete;
  ScalarMapCache& operator=(const ScalarMapCache&) = delete;

  // Returns cached map if available, else loads it synchronously.
  // `maxSize` limits the decoded base mip resolution (preserves aspect ratio).
  const Map* getOrLoadGrayscale(const std::string& path, int maxSize = 2048);

  // Samples a cached map (wraps UVs). Returns 0..1 if map is valid.
  static float sample01(const Map& m, float u, float v, int mip);

 private:
  static std::optional<Map> loadGrayscale(const std::string& path, int maxSize);
  static void buildMips(Map& out);

  std::mutex m_mutex;
  std::unordered_map<std::string, Map> m_cache;
};

}  // namespace terrain

