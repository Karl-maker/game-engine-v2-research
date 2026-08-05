#pragma once

// Author: Karl-Johan Bailey
//
// ColliderComponent (descriptive only)
// Describes collision shape and settings (not movement).
// A physics/collision system interprets this component.
//
// Terrain note:
// - Terrain colliders refer to a TerrainComponent entity id as their "source".
// - A terrain-height provider service (see `terrain::ITerrainHeightProvider`) can expose height sampling
//   based on TerrainComponent + noise settings.

#include "ecs/EntityId.h"
#include "math/Vec3.h"
#include "physics/LayerMask.h"

#include <string>

namespace ecs {

struct ColliderComponent {
  enum class Shape {
    Box,
    Sphere,
    Capsule,
    Mesh,
    Terrain,
  };

  enum class TerrainColliderType {
    Heightfield,
  };

  Shape shape = Shape::Box;

  // Generic size parameters (interpret based on `shape`).
  // - Box: (width, height, depth)
  // - Sphere: x = radius
  // - Capsule: x = radius, y = height
  math::Vec3 size{1.0f, 1.0f, 1.0f};

  math::Vec3 offset{0.0f, 0.0f, 0.0f};

  bool isTrigger = false;

  // Collision filtering (engine-defined).
  physics::LayerMask collisionLayer = physics::kAllLayers;

  // Mesh collider (engine-defined handle/id).
  bool hasMesh = false;
  std::uint32_t meshId = 0;
  std::string meshRef;
  std::string meshAssetKey;
  bool useMeshBounds = false;

  // Terrain collider settings.
  struct TerrainSource {
    bool enabled = false;
    EntityId sourceTerrainEntity = kInvalidEntityId;  // entity that has TerrainComponent
    TerrainColliderType colliderType = TerrainColliderType::Heightfield;
    physics::LayerMask collisionLayer = physics::kAllLayers;
    float thicknessMeters = 5.0f;
  } terrain;
};

}  // namespace ecs
