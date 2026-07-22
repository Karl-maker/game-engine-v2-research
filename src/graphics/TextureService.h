#pragma once

// Author: Karl-Johan Bailey
//
// TextureService (demo-focused)
// Asynchronously loads image files, decodes them, and uploads to OpenGL textures
// on the render thread.

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace graphics {

class TextureService final {
 public:
  struct TextureInfo final {
    std::uint32_t glId = 0;
    int width = 0;
    int height = 0;
    bool srgb = false;
    bool ready = false;
  };

  TextureService();
  ~TextureService();

  TextureService(const TextureService&) = delete;
  TextureService& operator=(const TextureService&) = delete;

  void start();
  void stop();

  // Returns cached GL texture id if ready, else 0. Also schedules load if needed.
  std::uint32_t requestTexture(const std::string& path, bool srgb);

  // Upload decoded textures to GPU. Must be called on the OpenGL context thread.
  void flushUploads(std::size_t maxUploadsPerFrame = 4);

  // Deletes all GL textures owned by this cache. Must be called on GL context thread.
  void destroyAllGlTextures();

  std::optional<TextureInfo> getInfo(const std::string& path) const;

 private:
  struct PendingDecode final {
    std::string path;
    bool srgb = false;
  };

  struct PendingUpload final {
    std::string path;
    bool srgb = false;
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;  // size = width*height*4
  };

  struct Entry final {
    enum class State { Unloaded, Queued, Decoding, Ready, Failed };
    State state = State::Unloaded;
    TextureInfo info{};
  };

  void workerMain();
  bool decodeImageRGBA(const std::string& path, int& outW, int& outH, std::vector<std::uint8_t>& outRgba) const;

  mutable std::mutex m_mutex;
  std::unordered_map<std::string, Entry> m_entries;
  std::vector<PendingDecode> m_decodeQueue;
  std::vector<PendingUpload> m_uploadQueue;

  std::thread m_worker;
  bool m_running = false;
};

}  // namespace graphics
