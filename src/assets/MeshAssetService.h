#pragma once

// Author: Karl-Johan Bailey
//
// MeshAssetService
// Async CPU loader for mesh assets. Loaders are selected by file extension so
// OBJ/FBX/etc can be plugged in without changing ECS components or renderer code.

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace assets {

struct MeshVertex final {
  float px = 0.0f, py = 0.0f, pz = 0.0f;
  float nx = 0.0f, ny = 1.0f, nz = 0.0f;
  float u = 0.0f, v = 0.0f;
  std::uint16_t joints[4]{};
  float weights[4]{};
};

struct MeshMaterial final {
  std::string name;
  math::Vec3 baseColorFactor{1.0f, 1.0f, 1.0f};
  math::Vec3 emissiveFactor{0.0f, 0.0f, 0.0f};
  float roughnessFactor = 1.0f;
  float metallicFactor = 1.0f;
  float normalScale = 1.0f;
  float occlusionStrength = 1.0f;
  float specularFactor = 1.0f;
  std::string baseColorTexture;
  std::string normalTexture;
  std::string metallicRoughnessTexture;
  std::string occlusionTexture;
  std::string emissiveTexture;
  std::string specularTexture;
};

struct LoadedSubMesh final {
  std::string name;
  std::uint32_t materialIndex = 0;
  std::vector<MeshVertex> vertices;
  std::vector<std::uint32_t> indices;
};

struct LoadedSkeleton final {
  struct Bone final {
    std::string name;
    int nodeIndex = -1;
    int parentIndex = -1;
    math::Mat4 localBindTransform{};
    math::Mat4 inverseBindMatrix{};
  };

  std::string id;
  int rootBone = -1;
  std::vector<Bone> bones;
};

struct LoadedAnimation final {
  enum class Path {
    Translation,
    Rotation,
    Scale,
  };

  struct Channel final {
    int boneIndex = -1;
    Path path = Path::Translation;
    std::vector<float> times;
    std::vector<math::Vec3> vec3Values;
    std::vector<math::Quat> quatValues;
  };

  std::string name;
  float durationSeconds = 0.0f;
  std::vector<Channel> channels;
};

struct LoadedMeshAsset final {
  std::string id;
  std::string sourcePath;
  std::vector<LoadedSubMesh> subMeshes;
  std::vector<MeshMaterial> materials;
  LoadedSkeleton skeleton;
  std::vector<LoadedAnimation> animations;
  math::Vec3 boundsMin{};
  math::Vec3 boundsMax{};
};

class IMeshAssetLoader {
 public:
  virtual ~IMeshAssetLoader() = default;
  virtual bool load(const std::string& path, LoadedMeshAsset& out, std::string& error) const = 0;
};

class MeshAssetService final {
 public:
  enum class State {
    Unloaded,
    Queued,
    Loading,
    Ready,
    Failed,
  };

  struct Status final {
    State state = State::Unloaded;
    std::string error;
  };

  MeshAssetService();
  ~MeshAssetService();

  MeshAssetService(const MeshAssetService&) = delete;
  MeshAssetService& operator=(const MeshAssetService&) = delete;

  void start();
  void stop();
  void registerLoader(std::string extension, std::unique_ptr<IMeshAssetLoader> loader);

  Status request(const std::string& path);
  std::optional<LoadedMeshAsset> takeReady(const std::string& path);
  Status status(const std::string& path) const;

 private:
  struct Entry final {
    State state = State::Unloaded;
    std::string error;
    std::optional<LoadedMeshAsset> ready;
  };

  void workerMain();
  const IMeshAssetLoader* loaderFor(const std::string& path) const;

  mutable std::mutex m_mutex;
  std::unordered_map<std::string, Entry> m_entries;
  std::unordered_map<std::string, std::unique_ptr<IMeshAssetLoader>> m_loaders;
  std::vector<std::string> m_queue;
  std::thread m_worker;
  bool m_running = false;
};

std::unique_ptr<IMeshAssetLoader> makeGltfMeshLoader();

}  // namespace assets
