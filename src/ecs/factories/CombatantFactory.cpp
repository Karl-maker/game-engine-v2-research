// Author: Karl-Johan Bailey

#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/ActorFactory.h"

#include "ecs/components/CharacterComponent.h"

namespace ecs::services {

EntityId CombatantFactory::create(
    EntityRegistry& registry,
    const CombatantConfig& config) {
  ActorFactory baseFactory;
  ActorConfig baseCfg{};
  baseCfg.transform = config.transform;
  baseCfg.viewable = config.viewable;
  baseCfg.physical = config.physical;
  baseCfg.skeleton = config.skeleton;
  baseCfg.stats = config.stats;
  baseCfg.animation = config.animation;
  baseCfg.pose = config.pose;
  baseCfg.ik = config.ik;
  baseCfg.sensorCone = config.sensorCone;
  const EntityId id = baseFactory.create(registry, baseCfg);
  if (id == kInvalidEntityId) return id;

  registry.emplace<ecs::CharacterComponent>(id);

  return id;
}

}  // namespace ecs::services
