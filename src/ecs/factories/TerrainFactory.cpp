// Author: Karl-Johan Bailey

#include "ecs/factories/TerrainFactory.h"

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"

#include "materials/presets/HighQualityDirtRockLayer.h"

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
  terrain.lodStep32Distance = config.lodStep32Distance;
  terrain.lodForceNearDistance = config.lodForceNearDistance;
  terrain.tessLockDistance = config.tessLockDistance;
  terrain.tessEnableDistance = config.tessEnableDistance;
  terrain.tessDisableDistance = config.tessDisableDistance;
  terrain.viewDotBias = config.viewDotBias;
  terrain.farLod.enabled = config.farLod.enabled;
  terrain.farLod.startDistance = config.farLod.startDistance;
  terrain.farLod.endDistance = config.farLod.endDistance;
  terrain.farLod.billboardScale = config.farLod.billboardScale;
  terrain.farLod.heightOffset = config.farLod.heightOffset;
  terrain.farLod.cameraFacing = config.farLod.cameraFacing;
  terrain.farLod.texture = config.farLod.texture;
  terrain.farLod.tint = config.farLod.tint;

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
    ecs::ShaderComponent base = config.material ? *config.material : materials::presets::HighQualityDirtRockLayer();
    base.shader.key = config.shaderKey;
    base.depthWrite = true;

    auto& shader = registry.emplace<ecs::ShaderComponent>(id, std::move(base));

    // Keep previous TerrainFactory defaults unless an explicit material is provided.
    if (!config.material) {
      shader.parameters.push_back({"roughness", 1.0f});
      shader.parameters.push_back({"metallic", 0.0f});
      shader.parameters.push_back({"specularIntensity", 0.05f});
    }

    appendViewableOverrides(config.viewable, shader);
  }

  return id;
}

}  // namespace ecs::services
