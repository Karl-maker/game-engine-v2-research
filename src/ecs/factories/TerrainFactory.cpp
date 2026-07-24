// Author: Karl-Johan Bailey

#include "ecs/factories/TerrainFactory.h"

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"
#include "materials/presets/HighQualityDirtRockLayer.h"
#include "physics/LayerMask.h"

namespace ecs::services {

EntityId TerrainFactory::create(EntityRegistry& registry, const TerrainConfig& config) {
  EntityId id = registry.createEntity(config.name.empty() ? "terrain" : config.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.position;

  auto& terrain = registry.emplace<ecs::TerrainComponent>(id);
  terrain.gridWidth = config.gridWidth;
  terrain.gridHeight = config.gridHeight;
  terrain.cellSizeMeters = config.cellSizeMeters;
  terrain.heightScaleMeters = config.heightScaleMeters;
  terrain.noise = config.noise;
  terrain.noiseSeed = config.noise.seed;
  terrain.lodMaxRenderDistance = config.lodMaxRenderDistance;
  terrain.lodStep1Distance = config.lodStep1Distance;
  terrain.lodStep2Distance = config.lodStep2Distance;
  terrain.lodStep4Distance = config.lodStep4Distance;
  terrain.lodStep8Distance = config.lodStep8Distance;
  terrain.lodStep16Distance = config.lodStep16Distance;
  terrain.lodForceNearDistance = config.lodForceNearDistance;
  terrain.tessLockDistance = config.tessLockDistance;
  terrain.tessEnableDistance = config.tessEnableDistance;
  terrain.tessDisableDistance = config.tessDisableDistance;
  terrain.viewDotBias = config.viewDotBias;

  if (config.colliderEnabled) {
    auto& collider = registry.emplace<ecs::ColliderComponent>(id);
    collider.shape = ecs::ColliderComponent::Shape::Terrain;
    collider.collisionLayer = physics::kLayerWorld;
    collider.terrain.enabled = true;
    collider.terrain.sourceTerrainEntity = id;
    collider.terrain.collisionLayer = physics::kLayerWorld;
    collider.terrain.thicknessMeters = config.colliderThicknessMeters;
  }

  ecs::ShaderComponent shader = materials::presets::HighQualityDirtRockLayer();
  shader.shader.key = config.shaderKey;
  shader.depthWrite = config.depthWrite;
  registry.emplace<ecs::ShaderComponent>(id, std::move(shader));

  return id;
}

}  // namespace ecs::services
