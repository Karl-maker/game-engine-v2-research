// Author: Karl-Johan Bailey

#include "ecs/factories/VfxFactory.h"

#include "ecs/components/TransformComponent.h"

namespace ecs::services {

EntityId VfxFactory::create(EntityRegistry& registry, const VfxConfig& config) {
  EntityId id = registry.createEntity(config.name.empty() ? "vfx" : config.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.position;
  tr.rotation = config.rotationDeg;
  tr.scale = config.scale;

  auto& vfx = registry.emplace<ecs::VfxComponent>(id);
  vfx = config.component;
  return id;
}

}  // namespace ecs::services
