#pragma once

// Author: Karl-Johan Bailey
//
// MeshComponent
// Describes the geometric model an entity owns. The renderer/asset services turn
// the references into GPU buffers without hard-coding asset paths into systems.

#include "math/Vec3.h"
#include "render/AssetRef.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ecs {

struct MeshComponent final {
  enum class MeshType {
    Static,
    Skinned,
    Procedural,
  };

  struct SubMesh final {
    std::string name;
    std::uint32_t materialIndex = 0;
    std::uint32_t indexOffset = 0;
    std::uint32_t indexCount = 0;
  };

  struct Bounds final {
    math::Vec3 min{};
    math::Vec3 max{};
  };

  bool enabled = true;
  std::string meshId;
  render::AssetRef meshData{};
  MeshType meshType = MeshType::Static;
  std::vector<SubMesh> subMeshes;
  Bounds bounds{};
  math::Vec3 pivot{0.0f, 0.0f, 0.0f};
  math::Vec3 scale{1.0f, 1.0f, 1.0f};
  bool visible = true;
  bool castShadows = true;
  bool receiveShadows = true;
  std::vector<std::string> tags;

  std::string skeletonId;
  std::string uvMappingTextureId;
};

}  // namespace ecs
