#pragma once

// Author: Karl-Johan Bailey
//
// Collision events are ephemeral physics messages. Detection emits contacts;
// resolution consumes contacts and emits the final correction that was applied.

#include "ecs/EntityId.h"
#include "math/Vec3.h"

namespace ecs::events {

struct CollisionDetectionEvent final {
  ecs::EntityId entityA = ecs::kInvalidEntityId;
  ecs::EntityId entityB = ecs::kInvalidEntityId;
  math::Vec3 contactPoint{};
  math::Vec3 contactNormal{0.0f, 1.0f, 0.0f};  // points from B toward A
  float penetrationDepth = 0.0f;
  double time = 0.0;
};

struct CollisionResolutionEvent final {
  ecs::EntityId entityA = ecs::kInvalidEntityId;
  ecs::EntityId entityB = ecs::kInvalidEntityId;
  math::Vec3 correctionA{};
  math::Vec3 correctionB{};
  math::Vec3 contactNormal{0.0f, 1.0f, 0.0f};
  float penetrationDepth = 0.0f;
  double time = 0.0;
};

}  // namespace ecs::events
