#pragma once

// Author: Karl-Johan Bailey
//
// RaycastHit (descriptive results container)
// Physics/raycast systems populate these fields.

#include "ecs/EntityId.h"
#include "math/Vec2.h"
#include "math/Vec3.h"

#include <cstdint>

namespace physics {

struct RaycastHit {
  bool hasHit = false;

  ecs::EntityId hitEntityId = ecs::kInvalidEntityId;
  math::Vec3 hitPosition{0.0f, 0.0f, 0.0f};
  math::Vec3 hitNormal{0.0f, 1.0f, 0.0f};
  float distance = 0.0f;

  // Engine-defined collider handle/id.
  std::uint32_t colliderId = 0;

  // Optional: Physics material handle/id.
  std::uint32_t physicsMaterialId = 0;

  // Optional mesh details (engine-defined).
  bool hasTriangleIndex = false;
  std::uint32_t triangleIndex = 0;

  bool hasUv = false;
  math::Vec2 uv{0.0f, 0.0f};

  // Optional time along the ray for continuous queries (0..1 or seconds; engine-defined).
  double hitTime = 0.0;
};

}  // namespace physics

