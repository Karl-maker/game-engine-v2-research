#pragma once

// Author: Karl-Johan Bailey
//
// Motion-related events emitted by MovementSystem.
// Events are ephemeral (cleared after a frame by EventService).

#include "ecs/EntityId.h"

namespace ecs::events {

struct MotionStartedEvent final {
  ecs::EntityId entity = ecs::kInvalidEntityId;
};

struct MotionStoppedEvent final {
  ecs::EntityId entity = ecs::kInvalidEntityId;
};

struct LandedEvent final {
  ecs::EntityId entity = ecs::kInvalidEntityId;
};

}  // namespace ecs::events

