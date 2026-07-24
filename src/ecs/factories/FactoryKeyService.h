#pragma once

// Author: Karl-Johan Bailey
//
// FactoryKeyService
// Registers string keys -> IEntityFactory adapters for factories in `src/ecs/factories`.

#include "ecs/services/EntityFactoryRegistry.h"

namespace ecs::services {

void registerFactoriesFromEcsFactoriesDir(EntityFactoryRegistry& out);

}  // namespace ecs::services

