#pragma once

// Author: Karl-Johan Bailey
//
// Ray and sensor events are ephemeral. Ray detection emits hits; sensor systems can turn them into alerts.

#include "ecs/EntityId.h"
#include "physics/RaycastHit.h"

#include <string>

namespace ecs::events {

struct RayHitEvent final {
  ecs::EntityId rayEntity = ecs::kInvalidEntityId;
  ecs::EntityId sensorEntity = ecs::kInvalidEntityId;
  ecs::EntityId hitEntity = ecs::kInvalidEntityId;
  std::string raycastCategory;
  physics::RaycastHit hit{};
};

struct SensorAlertEvent final {
  ecs::EntityId sensorEntity = ecs::kInvalidEntityId;
  ecs::EntityId parentEntity = ecs::kInvalidEntityId;
  ecs::EntityId rayEntity = ecs::kInvalidEntityId;
  ecs::EntityId hitEntity = ecs::kInvalidEntityId;
  physics::RaycastHit hit{};
  std::string raycastCategory;
};

}  // namespace ecs::events
