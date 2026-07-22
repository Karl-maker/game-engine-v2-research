#include "ecs/systems/GraphicsSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/CameraComponent.h"
#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/RockScatterComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"

#include "math/Vec3.h"
#include "render/Color.h"
#include "render/TextureBinding.h"

#include <cmath>
#include <variant>

namespace ecs::systems {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;

math::Vec3 forwardFromPitchYawDeg(const ecs::TransformComponent& tr) {
  const float pitch = tr.rotation.x * kDegToRad;
  const float yaw = tr.rotation.y * kDegToRad;

  // Convention:
  // - yaw=0, pitch=0 => +Z forward
  // - positive yaw turns toward +X
  // - positive pitch looks downward (screen-space typical), hence -sin(pitch) on Y
  const math::Vec3 fwd{
      std::cos(pitch) * std::sin(yaw),
      -std::sin(pitch),
      std::cos(pitch) * std::cos(yaw),
  };

  return math::normalize(fwd);
}

bool readFloatParam(const ecs::ShaderComponent& shader, const char* name, float& out) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<float>(&p.value)) {
      out = *v;
      found = true;
    }
  }
  return found;
}

bool readBaseColorParam(const ecs::ShaderComponent& shader, float& r, float& g, float& b) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != "baseColor") continue;
    if (const auto* c = std::get_if<render::Color>(&p.value)) {
      r = c->r;
      g = c->g;
      b = c->b;
      found = true;
    }
  }
  return found;
}

bool readBoolParam(const ecs::ShaderComponent& shader, const char* name, bool& out) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<bool>(&p.value)) {
      out = *v;
      found = true;
    }
  }
  return found;
}

bool readVec2Param(const ecs::ShaderComponent& shader, const char* name, float& x, float& y) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<math::Vec2>(&p.value)) {
      x = v->x;
      y = v->y;
      found = true;
    }
  }
  return found;
}

void extractKnownTextures(const ecs::ShaderComponent& shader, GraphicsSystem::TerrainDraw& out) {
  auto tryBind = [&](const char* slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        return;
      }
    }
  };

  tryBind("albedo", out.albedoTex, out.hasAlbedoTex);

  // Prefer OpenGL normal map if explicitly provided.
  tryBind("normalgl", out.normalTex, out.hasNormalTex);
  if (!out.hasNormalTex) {
    tryBind("normal", out.normalTex, out.hasNormalTex);
  }

  tryBind("roughness", out.roughnessTex, out.hasRoughnessTex);
  tryBind("ao", out.aoTex, out.hasAoTex);
  tryBind("ambient_occlusion", out.aoTex, out.hasAoTex);
  tryBind("displacement", out.displacementTex, out.hasDisplacementTex);
}

void extractRockLayerTextures(const ecs::ShaderComponent& shader, GraphicsSystem::TerrainDraw& out) {
  auto tryBind = [&](const char* slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        return;
      }
    }
  };

  tryBind("rock_albedo", out.rockAlbedoTex, out.hasRockAlbedoTex);
  tryBind("rock_normalgl", out.rockNormalTex, out.hasRockNormalTex);
  tryBind("rock_roughness", out.rockRoughnessTex, out.hasRockRoughnessTex);
  tryBind("rock_ao", out.rockAoTex, out.hasRockAoTex);
  tryBind("rock_displacement", out.rockDisplacementTex, out.hasRockDisplacementTex);
}

}  // namespace

const GraphicsSystem::FrameSnapshot& GraphicsSystem::tick(EntityRegistry& registry) {
  m_frame.camera = {};
  m_frame.terrains.clear();
  m_frame.fogVolumes.clear();
  m_frame.skies.clear();
  m_frame.rocks.clear();
  m_frame.lights.clear();

  // --- Camera (pick the first available) ---
  registry.view<ecs::CameraComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::CameraComponent& cam, const ecs::TransformComponent& tr) {
        if (m_frame.camera.entity != ecs::kInvalidEntityId) return;
        m_frame.camera.entity = id;
        m_frame.camera.position = tr.position;
        m_frame.camera.forward = forwardFromPitchYawDeg(tr);
        m_frame.camera.fovYRadians = cam.fieldOfViewDeg * kDegToRad;
        m_frame.camera.nearClip = cam.nearClipPlane;
        m_frame.camera.farClip = cam.farClipPlane;
      });

  // --- Terrains with shaders ---
  registry.view<ecs::TerrainComponent, ecs::ShaderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::TerrainComponent& terrain,
          const ecs::ShaderComponent& shader,
          const ecs::TransformComponent& tr) {
        if (!shader.enabled) return;
        TerrainDraw draw;
        draw.entity = id;
        draw.position = tr.position;
        draw.gridWidth = terrain.gridWidth;
        draw.gridHeight = terrain.gridHeight;
        draw.cellSizeMeters = terrain.cellSizeMeters;
        draw.heightScaleMeters = terrain.heightScaleMeters;
        draw.noise = terrain.noise;
        draw.noiseSeed = static_cast<std::uint32_t>(id) * 1337u;
        draw.shader = shader.shader;
        draw.renderMode = static_cast<int>(shader.renderMode);
        draw.cullMode = static_cast<int>(shader.cullMode);
        draw.depthTest = static_cast<int>(shader.depthTest);
        draw.blendMode = static_cast<int>(shader.blendMode);
        draw.depthWrite = shader.depthWrite;
        draw.doubleSided = shader.doubleSided;
        draw.receiveShadows = shader.receiveShadows;
        draw.castShadows = shader.castShadows;
        draw.textureCount = shader.textures.size();
        draw.parameterCount = shader.parameters.size();

        (void)readBaseColorParam(shader, draw.baseColorR, draw.baseColorG, draw.baseColorB);
        (void)readFloatParam(shader, "roughness", draw.roughness);
        (void)readFloatParam(shader, "metallic", draw.metallic);
        (void)readFloatParam(shader, "specularIntensity", draw.specularIntensity);
        (void)readFloatParam(shader, "dirtColorNoiseStrength", draw.dirtColorNoiseStrength);
        (void)readBoolParam(shader, "dirtSinksEnabled", draw.dirtSinksEnabled);
        (void)readFloatParam(shader, "dirtSinkStrength", draw.dirtSinkStrength);
        (void)readFloatParam(shader, "dirtSinkScale", draw.dirtSinkScale);
        (void)readFloatParam(shader, "dirtSinkDensity", draw.dirtSinkDensity);

        extractKnownTextures(shader, draw);
        (void)readVec2Param(shader, "uvTiling", draw.uvTilingX, draw.uvTilingY);
        (void)readFloatParam(shader, "normalScale", draw.normalStrength);
        (void)readFloatParam(shader, "aoStrength", draw.aoStrength);
        (void)readFloatParam(shader, "displacementStrength", draw.displacementStrength);

        (void)readBoolParam(shader, "rockLayerEnabled", draw.rockLayerEnabled);
        extractRockLayerTextures(shader, draw);
        (void)readVec2Param(shader, "rockUvTiling", draw.rockUvTilingX, draw.rockUvTilingY);
        (void)readFloatParam(shader, "rockNormalScale", draw.rockNormalStrength);
        (void)readFloatParam(shader, "rockDisplacementStrength", draw.rockDisplacementStrength);
        (void)readFloatParam(shader, "rockBlendStrength", draw.rockBlendStrength);
        (void)readFloatParam(shader, "rockNoiseScale", draw.rockNoiseScale);
        m_frame.terrains.push_back(std::move(draw));
      });

  // --- Lights ---
  registry.view<ecs::LightComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::LightComponent& light, const ecs::TransformComponent& tr) {
        if (!light.enabled) return;
        LightDraw draw;
        draw.entity = id;
        draw.type = static_cast<int>(light.type);
        draw.position = tr.position;
        draw.direction = light.direction;
        draw.color = light.color;
        draw.intensity = light.intensity;
        draw.range = light.range;
        draw.castShadows = light.castShadows;
        m_frame.lights.push_back(std::move(draw));
      });

  // --- Fog volumes (pick the first enabled) ---
  registry.view<ecs::FogVolumeComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::FogVolumeComponent& fog, const ecs::TransformComponent& tr) {
        if (!fog.enabled) return;
        if (!m_frame.fogVolumes.empty()) return;
        FrameSnapshot::FogDraw draw;
        draw.entity = id;
        draw.center = tr.position;
        draw.sizeMeters = fog.sizeMeters;
        draw.color = fog.color;
        draw.density = fog.density;
        draw.startDistance = fog.startDistance;
        draw.endDistance = fog.endDistance;
        draw.heightFalloff = fog.heightFalloff;
        draw.baseHeightOffset = fog.baseHeightOffset;
        m_frame.fogVolumes.push_back(std::move(draw));
      });

  // --- Skies (pick the first enabled) ---
  registry.view<ecs::SkyComponent, ecs::ShaderComponent>(
      [&](ecs::EntityId id, const ecs::SkyComponent& sky, const ecs::ShaderComponent& shader) {
        if (!sky.enabled) return;
        if (!shader.enabled) return;
        if (!m_frame.skies.empty()) return;
        FrameSnapshot::SkyDraw draw;
        draw.entity = id;
        draw.shader = shader.shader;
        draw.horizonColor = sky.horizonColor;
        draw.zenithColor = sky.zenithColor;
        draw.sunEnabled = sky.sunEnabled;
        draw.sunDirection = sky.sunDirection;
        draw.sunTint = sky.sunTint;
        draw.sunDiscIntensity = sky.sunDiscIntensity;
        draw.sunDiscSize = sky.sunDiscSize;
        draw.cloudsEnabled = sky.cloudsEnabled;
        draw.skyType = static_cast<int>(sky.skyType);
        draw.cloudType = static_cast<int>(sky.cloudType);
        draw.quality = static_cast<int>(sky.quality);
        draw.cloudCoverage = sky.cloudCoverage;
        draw.cloudDensity = sky.cloudDensity;
        draw.cloudSpeed = sky.cloudSpeed;
        draw.cloudWindX = sky.cloudWindDirection.x;
        draw.cloudWindZ = sky.cloudWindDirection.y;
        draw.cloudTimeScale = sky.cloudTimeScale;
        draw.cloudTurbulence = sky.cloudTurbulence;
        draw.cloudScale = sky.cloudScale;
        draw.cloudLightAbsorption = sky.cloudLightAbsorption;
        draw.cloudHeightMeters = sky.cloudHeightMeters;

        draw.starsEnabled = sky.starsEnabled;
        draw.starsIntensity = sky.starsIntensity;
        draw.starsDensity = sky.starsDensity;
        draw.starsSize = sky.starsSize;
        draw.starsTwinkleStrength = sky.starsTwinkleStrength;
        draw.starsTwinkleSpeed = sky.starsTwinkleSpeed;
        draw.starsSeed = sky.starsSeed;
        m_frame.skies.push_back(std::move(draw));
      });

  // --- Rock scatters with shaders ---
  registry.view<ecs::RockScatterComponent, ecs::ShaderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::RockScatterComponent& rocks,
          const ecs::ShaderComponent& shader,
          const ecs::TransformComponent& tr) {
        if (!rocks.enabled) return;
        if (!shader.enabled) return;
        FrameSnapshot::RockDraw draw;
        draw.entity = id;
        draw.position = tr.position;
        draw.area = rocks.area;
        draw.density = rocks.density;
        draw.seed = rocks.seed;
        draw.minScale = rocks.minScale;
        draw.maxScale = rocks.maxScale;
        draw.clumpiness = rocks.clumpiness;
        draw.patchScale = rocks.patchScale;
        draw.lodBias = rocks.lodBias;
        draw.castShadows = rocks.castShadows;
        draw.receiveShadows = rocks.receiveShadows;
        draw.shader = shader.shader;
        m_frame.rocks.push_back(std::move(draw));
      });

  return m_frame;
}

}  // namespace ecs::systems
