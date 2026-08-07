#pragma once

// Author: Karl-Johan Bailey
//
// FactoryKeyService
// Registers string keys -> IEntityFactory adapters for factories in `src/ecs/factories`.

#include "data/Json.h"
#include "ecs/factories/PlayableCharacterFactory.h"
#include "ecs/services/EntityFactoryRegistry.h"
#include "ecs/services/IEntityFactory.h"

namespace ecs::services {

PlayableCharacterConfig readPlayableCharacterInput(const data::JsonValue::Object& obj, const FactoryContext& ctx);

void registerFactoriesFromEcsFactoriesDir(EntityFactoryRegistry& out);

}  // namespace ecs::services
