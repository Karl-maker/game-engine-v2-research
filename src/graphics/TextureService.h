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

  struct GreenMask final {
    int width = 0;
    int height = 0;
    bool ready = false;
    std::vector<std::uint8_t> green;  // size = width*height, 0..255
  };

  TextureService();
  ~TextureService();

  TextureService(const TextureService&) = delete;
  TextureService& operator=(const TextureService&) = delete;

  void start();
  void stop();

  // Returns cached GL texture id if ready, else 0. Also schedules load if needed.
  std::uint32_t requestTexture(const std::string& path, bool srgb);

  // Schedules a CPU-side downsampled green-channel mask build for the texture.
  void requestGreenMask(const std::string& path);

  // Upload decoded textures to GPU. Must be called on the OpenGL context thread.
  void flushUploads(std::size_t maxUploadsPerFrame = 4);

  // Deletes all GL textures owned by this cache. Must be called on GL context thread.
  void destroyAllGlTextures();

  std::optional<TextureInfo> getInfo(const std::string& path) const;

  bool hasGreenMaskReady(const std::string& path) const;
  std::optional<float> sampleGreenMask(const std::string& path, float u, float v) const;

 private:
  struct PendingDecode final {
    std::string path;
    bool srgb = false;
  };

  struct PendingMaskDecode final {
    std::string path;
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
    GreenMask greenMask{};
    bool greenMaskQueued = false;
    bool wantGreenMask = false;
  };

  void workerMain();
  bool decodeImageRGBA(const std::string& path, int& outW, int& outH, std::vector<std::uint8_t>& outRgba) const;

  mutable std::mutex m_mutex;
  std::unordered_map<std::string, Entry> m_entries;
  std::vector<PendingDecode> m_decodeQueue;
  std::vector<PendingMaskDecode> m_maskDecodeQueue;
  std::vector<PendingUpload> m_uploadQueue;

  std::thread m_worker;
  bool m_running = false;
};

}  // namespace graphics
