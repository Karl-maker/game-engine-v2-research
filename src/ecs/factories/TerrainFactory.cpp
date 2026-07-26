// Author: Karl-Johan Bailey

#include "ecs/factories/TerrainFactory.h"

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>
#include <utility>

namespace ecs::services {

namespace {

void appendShaderBreakpoints(const ViewableInput& viewable, ecs::ShaderComponent& shader) {
  shader.lodBreakpoints.reserve(shader.lodBreakpoints.size() + viewable.lodBreakpoints.size());
  for (const auto& bp : viewable.lodBreakpoints) {
    ecs::ShaderComponent::LodBreakpoint out{};
    out.distanceMeters = bp.distanceMeters;
    out.textures = bp.textures;
    out.parameters = bp.parameters;
    out.overrideTessellation = bp.overrideTessellation;
    out.tessNear = bp.tessNear;
    out.tessFar = bp.tessFar;
    out.tessMin = bp.tessMin;
    out.tessMax = bp.tessMax;
    out.tessQuality = bp.tessQuality;
    shader.lodBreakpoints.push_back(std::move(out));
  }
}

void appendViewableOverrides(const ViewableInput& viewable, ecs::ShaderComponent& shader) {
  shader.castShadows = shader.castShadows && viewable.castShadows;
  shader.receiveShadows = shader.receiveShadows && viewable.receiveShadows;

  shader.textures.insert(shader.textures.begin(), viewable.textures.begin(), viewable.textures.end());
  shader.parameters.insert(shader.parameters.end(), viewable.parameters.begin(), viewable.parameters.end());
  appendShaderBreakpoints(viewable, shader);
}

}  // namespace

EntityId TerrainFactory::create(EntityRegistry& registry, const TerrainConfig& config) {
  EntityId id = registry.createEntity(config.transform.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.transform.position;
  tr.rotation = config.transform.rotationDeg;
  tr.scale = config.transform.scale;

  auto& terrain = registry.emplace<ecs::TerrainComponent>(id);
  terrain.gridWidth = std::max(2, config.gridWidth);
  terrain.gridHeight = std::max(2, config.gridHeight);
  terrain.cellSizeMeters = std::max(0.01f, config.cellSizeMeters);
  terrain.heightScaleMeters = config.heightScaleMeters;
  terrain.noiseSeed = config.noiseSeed;
  terrain.noise = config.noise;
  terrain.noise.seed = config.noiseSeed;
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

  if (config.hasCollider) {
    auto& collider = registry.emplace<ecs::ColliderComponent>(id);
    collider.shape = ecs::ColliderComponent::Shape::Terrain;
    collider.collisionLayer = config.collisionLayer;
    collider.terrain.enabled = true;
    collider.terrain.sourceTerrainEntity = id;
    collider.terrain.collisionLayer = config.collisionLayer;
    collider.terrain.thicknessMeters = std::max(0.01f, config.colliderThicknessMeters);
  }

  if (config.hasShader) {
    ecs::ShaderComponent shader = config.material.value_or(ecs::ShaderComponent{});
    if (!config.shaderKey.empty()) {
      shader.shader.key = config.shaderKey;
    }
    appendViewableOverrides(config.viewable, shader);
    registry.emplace<ecs::ShaderComponent>(id, std::move(shader));
  }

  return id;
}

}  // namespace ecs::services
