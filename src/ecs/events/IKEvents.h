#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"

#include <string>

namespace ecs::events {

struct IKChainSolvedEvent final {
  ecs::EntityId entity = ecs::kInvalidEntityId;
  std::string chainName;
};

}  // namespace ecs::events
