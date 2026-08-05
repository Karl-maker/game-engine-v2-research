// Author: Karl-Johan Bailey

#include "ecs/factories/BillboardFactory.h"

#include "ecs/components/BillboardComponent.h"
#include "ecs/components/TransformComponent.h"

namespace ecs::services {

EntityId BillboardFactory::create(EntityRegistry& registry, const BillboardConfig& config) {
  EntityId id = registry.createEntity(config.billboard.transform.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.billboard.transform.position;
  tr.rotation = config.billboard.transform.rotationDeg;
  tr.scale = config.billboard.transform.scale;

  auto& billboard = registry.emplace<ecs::BillboardComponent>(id);
  billboard.enabled = config.billboard.enabled;
  billboard.visible = config.billboard.visible;
  billboard.textureEnabled = config.billboard.textureEnabled;
  billboard.depthWrite = config.billboard.depthWrite;
  billboard.doubleSided = config.billboard.doubleSided;
  billboard.faceMode = config.billboard.faceMode;
  billboard.sizeMeters = config.billboard.sizeMeters;
  billboard.pivot = config.billboard.pivot;
  billboard.worldOffset = config.billboard.worldOffset;
  billboard.rotationOffsetDeg = config.billboard.rotationOffsetDeg;
  billboard.maxRenderDistance = config.billboard.maxRenderDistance;
  billboard.texture = config.billboard.texture;
  billboard.animatedTexture = config.billboard.animatedTexture;
  billboard.tint = config.billboard.tint;

  return id;
}

}  // namespace ecs::services
