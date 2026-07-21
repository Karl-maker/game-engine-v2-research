#pragma once

// Author: Karl-Johan Bailey
//
// IdentityComponent
// - A stable numeric id (monotonic, +1 allocation).
// - A human-friendly name.
//
// This is intended to be serializable (DB/config) and is safe to include on every entity.

#include "../EntityId.h"

#include <string>

namespace ecs {

struct IdentityComponent {
  EntityId id = kInvalidEntityId;
  std::string name;
};

}  // namespace ecs

