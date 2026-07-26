// Author: Karl-Johan Bailey

// ActorFactory
// Spawns a lightweight world "actor" (no rigidbody/collider by default).

#include "ecs/factories/ObjectFactory.h"

#include "ecs/components/MeshComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TransformComponent.h"

namespace ecs::services {

EntityId ObjectFactory::create(
    EntityRegistry& registry,
    const ObjectConfig& config) {

  EntityId id = registry.createEntity(config.transform.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.transform.position;
  tr.rotation = config.transform.rotationDeg;
  tr.scale = config.transform.scale;

  if (config.viewable.meshKey.empty()) {
    return id;
  }

  auto& mesh = registry.emplace<ecs::MeshComponent>(id);
  mesh.meshId = config.viewable.meshId.empty() ? config.transform.name : config.viewable.meshId;
  mesh.meshData.enabled = true;
  mesh.meshData.key = config.viewable.meshKey;
  mesh.meshType = config.viewable.meshType;
  mesh.scale = config.viewable.meshScale;
  mesh.skeletonId = config.viewable.skeletonId;
  mesh.visible = config.viewable.visible;
  mesh.castShadows = config.viewable.castShadows;
  mesh.receiveShadows = config.viewable.receiveShadows;
  mesh.tags = config.viewable.tags;

  auto& shader = registry.emplace<ecs::ShaderComponent>(id);
  shader.shader.key = config.viewable.shaderKey;
  shader.castShadows = config.viewable.castShadows;
  shader.receiveShadows = config.viewable.receiveShadows;

  return id;
}

}  // namespace ecs::services
