// Author: Karl-Johan Bailey

#include "ecs/factories/ActorFactory.h"
#include "ecs/factories/CombatantFactory.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/AudioComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId CombatantFactory::create(
    EntityRegistry& registry,
    const CombatantConfig& config) {
  ActorFactory actorFactory;
  ActorConfig actorCfg{};
  actorCfg.transform = config.transform;
  actorCfg.viewable = config.viewable;
  EntityId id = actorFactory.create(registry, actorCfg);
  if (id == kInvalidEntityId) return id;

  if (config.viewable.meshKey.empty()) {
    return id;
  }

  // @TODO - Add HUD
  // @TODO - Combat AI Behaviour

  // @TODO - If person, or humanoid, or mystical or monster then I will attach additional

  return id;
}

}  // namespace ecs::services
