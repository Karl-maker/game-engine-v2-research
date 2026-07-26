// Author: Karl-Johan Bailey

#include "ecs/factories/VfxFactory.h"

#include "ecs/components/TransformComponent.h"

namespace ecs::services {

EntityId VfxFactory::create(EntityRegistry& registry, const VfxConfig& config) {
  EntityId id = registry.createEntity(config.transform.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.transform.position;
  tr.rotation = config.transform.rotationDeg;
  tr.scale = config.transform.scale;

  registry.emplace<ecs::VfxComponent>(id) = config.vfx;
  return id;
}

}  // namespace ecs::services
