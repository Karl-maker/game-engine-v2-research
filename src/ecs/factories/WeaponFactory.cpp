// Author: Karl-Johan Bailey

// ActorFactory
// Spawns a lightweight world "actor" (no rigidbody/collider by default).

#include "ecs/factories/PhysicalObjectFactory.h"
#include "ecs/factories/WeaponFactory.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/RigidbodyComponent.h"

#include <algorithm>

namespace ecs::services {

EntityId WeaponFactory::create(
    EntityRegistry& registry,
    const WeaponConfig& config) {
  PhysicalObjectFactory objectFactory;
  PhysicalObjectConfig objectCfg{};
  objectCfg.transform = config.transform;
  objectCfg.viewable = config.viewable;
  objectCfg.physical = config.physical;
  EntityId id = objectFactory.create(registry, objectCfg);
  if (id == kInvalidEntityId) return id;

  // @TODO - Add Holster Setup Component

  return id;
}

}  // namespace ecs::services
