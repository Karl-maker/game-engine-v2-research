// Author: Karl-Johan Bailey

#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/ActorFactory.h"

#include "ecs/components/AudioComponent.h"
#include "ecs/components/CharacterComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId CombatantFactory::create(
    EntityRegistry& registry,
    const CombatantConfig& config) {
  ActorFactory objectFactory;
  ActorConfig objectCfg{};
  objectCfg.transform = config.transform;
  objectCfg.viewable = config.viewable;
  objectCfg.physical = config.physical;
  objectCfg.skeleton = config.skeleton;
  objectCfg.stats = config.stats;
  EntityId id = registry.createEntity(config.transform.name);
  if (id == kInvalidEntityId) return id;

  registry.emplace<ecs::CharacterComponent>(id);

  return id;
}

}  // namespace ecs::services
