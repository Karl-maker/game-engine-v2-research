// Author: Karl-Johan Bailey

// ActorFactory
// Spawns a lightweight world "actor" (no rigidbody/collider by default).

#include "ecs/factories/GrassPatchFactory.h"

#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/GrassPatchComponent.h"
#include "ecs/components/TerrainComponent.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace ecs::services {

namespace {

EntityId findTerrainContainingPointXZ(
    EntityRegistry& registry,
    const math::Vec3& worldPos) {
  EntityId best = kInvalidEntityId;
  float bestDist2 = std::numeric_limits<float>::infinity();

  registry.view<ecs::TerrainComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::TerrainComponent& terrain, const ecs::TransformComponent& tr) {
        const float sizeX = static_cast<float>(std::max(2, terrain.gridWidth)) * terrain.cellSizeMeters;
        const float sizeZ = static_cast<float>(std::max(2, terrain.gridHeight)) * terrain.cellSizeMeters;
        const float halfX = 0.5f * sizeX;
        const float halfZ = 0.5f * sizeZ;

        if (worldPos.x < tr.position.x - halfX) return;
        if (worldPos.x > tr.position.x + halfX) return;
        if (worldPos.z < tr.position.z - halfZ) return;
        if (worldPos.z > tr.position.z + halfZ) return;

        const float dx = worldPos.x - tr.position.x;
        const float dz = worldPos.z - tr.position.z;
        const float dist2 = dx * dx + dz * dz;
        if (dist2 < bestDist2) {
          bestDist2 = dist2;
          best = id;
        }
      });

  return best;
}

void appendShaderBreakpoints(const std::vector<ShaderBreakpointInput>& in, ecs::ShaderComponent& out) {
  out.lodBreakpoints.reserve(out.lodBreakpoints.size() + in.size());
  for (const auto& bp : in) {
    ecs::ShaderComponent::LodBreakpoint b{};
    b.distanceMeters = bp.distanceMeters;
    b.textures = bp.textures;
    b.parameters = bp.parameters;
    b.overrideTessellation = bp.overrideTessellation;
    b.tessNear = bp.tessNear;
    b.tessFar = bp.tessFar;
    b.tessMin = bp.tessMin;
    b.tessMax = bp.tessMax;
    b.tessQuality = bp.tessQuality;
    out.lodBreakpoints.push_back(std::move(b));
  }
}

}  // namespace

EntityId GrassPatchFactory::create(
    EntityRegistry& registry,
    const GrassPatchConfig& config) {

  EntityId id = registry.createEntity(config.transform.name);
  if (id == kInvalidEntityId) return id;

  auto& tr = registry.emplace<ecs::TransformComponent>(id);
  tr.position = config.transform.position;
  tr.rotation = config.transform.rotationDeg;
  tr.scale = config.transform.scale;

  auto& gp = registry.emplace<ecs::GrassPatchComponent>(id);
  gp.enabled = config.grass.enabled;
  gp.area = config.grass.area;
  gp.densityMultiplier = config.grass.densityMultiplier;
  gp.seed = config.grass.seed;
  gp.densityNoise = config.grass.densityNoise;
  gp.densityNoiseThreshold = config.grass.densityNoiseThreshold;
  gp.densityNoiseContrast = config.grass.densityNoiseContrast;
  gp.densityNoiseStrength = config.grass.densityNoiseStrength;
  gp.islandNoise = config.grass.islandNoise;
  gp.islandNoiseOffset = config.grass.islandNoiseOffset;
  gp.islandNoiseThreshold = config.grass.islandNoiseThreshold;
  gp.islandNoiseSoftness = config.grass.islandNoiseSoftness;
  gp.islandNoiseContrast = config.grass.islandNoiseContrast;
  gp.islandNoiseStrength = config.grass.islandNoiseStrength;
  gp.sourceTerrainEntity = config.grass.sourceTerrainEntity;
  gp.interactionEnabled = config.grass.interactionEnabled;
  gp.interactionRadiusMeters = config.grass.interactionRadiusMeters;
  gp.interactionStrength = config.grass.interactionStrength;
  gp.castShadows = config.grass.castShadows;
  gp.receiveShadows = config.grass.receiveShadows;
  gp.lodBias = config.grass.lodBias;

  gp.layers.clear();
  gp.layers.reserve(config.grass.layers.size());
  for (const auto& l : config.grass.layers) {
    ecs::GrassPatchComponent::GrassLayer out{};
    out.species = l.species;
    out.description = l.description;
    out.density = l.density;
    out.minScale = l.minScale;
    out.maxScale = l.maxScale;
    out.bladeSpacing = l.bladeSpacing;
    out.bendStrength = l.bendStrength;
    out.curveStrength = l.curveStrength;
    out.twistStrength = l.twistStrength;
    out.minSlopeDeg = l.minSlopeDeg;
    out.maxSlopeDeg = l.maxSlopeDeg;
    out.minAltitude = l.minAltitude;
    out.maxAltitude = l.maxAltitude;
    out.noiseScale = l.noiseScale;
    out.noiseStrength = l.noiseStrength;
    out.windStrength = l.windStrength;
    out.maxDistance = l.maxDistance;
    gp.layers.push_back(std::move(out));
  }

  if (gp.sourceTerrainEntity == kInvalidEntityId && config.grass.autoBindTerrain) {
    gp.sourceTerrainEntity = findTerrainContainingPointXZ(registry, tr.position);
  }

  if (config.grass.hasShader) {
    auto& sh = registry.emplace<ecs::ShaderComponent>(id);
    sh.shader.key = config.grass.shaderKey;
    sh.doubleSided = config.grass.doubleSided;
    sh.depthWrite = config.grass.depthWrite;
    sh.receiveShadows = config.grass.receiveShadows;
    sh.castShadows = config.grass.castShadows;
    sh.textures = config.grass.textures;
    sh.parameters = config.grass.parameters;
    appendShaderBreakpoints(config.grass.lodBreakpoints, sh);

    if (sh.textures.empty()) {
      sh.textures.push_back(render::TextureBinding{
          .slot = "grass_tex0",
          .texture = render::AssetRef{.enabled = true, .key = "assets/textures/vegitation/grass_patch_02/Material_baseColor.png"},
          .srgb = true,
      });
    }
  }

  return id;
}

}  // namespace ecs::services
