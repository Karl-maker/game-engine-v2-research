#include "ecs/systems/SkyPresetSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/SkyComponent.h"
#include "math/Vec3.h"

namespace {

static math::Vec3 neg(const math::Vec3& v) { return math::Vec3{-v.x, -v.y, -v.z}; }

struct SkyPreset final {
  render::Color horizon;
  render::Color zenith;
  bool sunEnabled = true;
  math::Vec3 sunDir{0.2f, 0.9f, 0.2f};  // direction the sun appears in (world-space)
  render::Color sunTint{1.0f, 0.95f, 0.85f, 1.0f};
  float sunDiscIntensity = 1.0f;
  float sunDiscSize = 1.0f;
  bool starsEnabled = false;

  math::Vec3 lightColor{1.0f, 0.96f, 0.88f};
  float lightIntensity = 3.25f;

  render::Color fogColor{0.55f, 0.62f, 0.72f, 1.0f};
  float fogDensity = 0.060f;
  float fogStart = 6.0f;
  float fogEnd = 160.0f;
  float fogHeightFalloff = 0.045f;
  float fogBaseHeightOffset = -4.0f;
};

static SkyPreset presetFor(ecs::SkyComponent::SkyType type) {
  SkyPreset p{};

  // Day defaults.
  p.horizon = {0.65f, 0.75f, 0.95f, 1.0f};
  p.zenith = {0.12f, 0.22f, 0.45f, 1.0f};
  p.sunDir = math::normalize(math::Vec3{0.2f, 0.9f, 0.2f});
  p.sunTint = {1.0f, 0.95f, 0.85f, 1.0f};
  p.sunDiscIntensity = 1.0f;
  p.sunDiscSize = 1.0f;
  p.starsEnabled = false;
  p.lightColor = {1.0f, 0.96f, 0.88f};
  p.lightIntensity = 3.25f;
  p.fogColor = {0.55f, 0.62f, 0.72f, 1.0f};
  p.fogDensity = 0.060f;
  p.fogStart = 6.0f;
  p.fogEnd = 160.0f;
  p.fogHeightFalloff = 0.045f;

  if (type == ecs::SkyComponent::SkyType::Sunset) {
    p.horizon = {0.92f, 0.42f, 0.18f, 1.0f};
    p.zenith = {0.18f, 0.16f, 0.32f, 1.0f};
    p.sunTint = {1.0f, 0.62f, 0.25f, 1.0f};
    p.sunDiscIntensity = 1.15f;
    p.lightColor = {1.0f, 0.62f, 0.30f};
    p.lightIntensity = 2.75f;
    p.fogColor = {0.72f, 0.42f, 0.30f, 1.0f};
    p.fogDensity = 0.085f;
  } else if (type == ecs::SkyComponent::SkyType::Night) {
    p.horizon = {0.04f, 0.06f, 0.13f, 1.0f};
    p.zenith = {0.015f, 0.03f, 0.09f, 1.0f};
    // "Moon" disc.
    p.sunDir = math::normalize(math::Vec3{-0.15f, 0.9f, -0.15f});
    p.sunTint = {0.72f, 0.80f, 1.0f, 1.0f};
    p.sunDiscIntensity = 0.75f;
    p.sunDiscSize = 0.65f;
    p.starsEnabled = true;
    p.lightColor = {0.78f, 0.86f, 1.0f};
    p.lightIntensity = 1.45f;
    p.fogColor = {0.05f, 0.07f, 0.12f, 1.0f};
    p.fogDensity = 0.090f;
    p.fogStart = 4.0f;
    p.fogEnd = 140.0f;
    p.fogHeightFalloff = 0.030f;
  } else if (type == ecs::SkyComponent::SkyType::Overcast) {
    p.horizon = {0.62f, 0.66f, 0.70f, 1.0f};
    p.zenith = {0.35f, 0.38f, 0.42f, 1.0f};
    p.sunDiscIntensity = 0.55f;
    p.sunTint = {0.85f, 0.88f, 0.92f, 1.0f};
    p.lightColor = {0.88f, 0.90f, 0.94f};
    p.lightIntensity = 2.10f;
    p.fogColor = {0.68f, 0.70f, 0.74f, 1.0f};
    p.fogDensity = 0.125f;
  } else if (type == ecs::SkyComponent::SkyType::Storm) {
    p.horizon = {0.22f, 0.24f, 0.28f, 1.0f};
    p.zenith = {0.06f, 0.08f, 0.11f, 1.0f};
    p.sunEnabled = false;
    p.starsEnabled = false;
    p.lightColor = {0.72f, 0.76f, 0.82f};
    p.lightIntensity = 1.65f;
    p.fogColor = {0.16f, 0.18f, 0.21f, 1.0f};
    p.fogDensity = 0.180f;
    p.fogStart = 2.0f;
    p.fogEnd = 110.0f;
    p.fogHeightFalloff = 0.020f;
  }

  return p;
}

static void applySkyPreset(ecs::SkyComponent& sky, const SkyPreset& p) {
  sky.horizonColor = p.horizon;
  sky.zenithColor = p.zenith;
  sky.sunEnabled = p.sunEnabled;
  sky.sunDirection = p.sunDir;
  sky.sunTint = p.sunTint;
  sky.sunDiscIntensity = p.sunDiscIntensity;
  sky.sunDiscSize = p.sunDiscSize;
  sky.starsEnabled = p.starsEnabled;
}

static void applyLightPreset(ecs::LightComponent& light, const ecs::SkyComponent& sky, const SkyPreset& p) {
  if (light.type != ecs::LightComponent::Type::Directional) return;
  light.enabled = true;
  // Light direction points "from light" (renderer uses -dir as the to-light vector).
  light.direction = math::normalize(neg(sky.sunDirection));
  light.color = p.lightColor;
  light.intensity = p.lightIntensity;
}

static void applyFogPreset(ecs::FogVolumeComponent& fog, const SkyPreset& p) {
  fog.enabled = true;
  fog.color = p.fogColor;
  fog.density = p.fogDensity;
  fog.startDistance = p.fogStart;
  fog.endDistance = p.fogEnd;
  fog.heightFalloff = p.fogHeightFalloff;
  fog.baseHeightOffset = p.fogBaseHeightOffset;
}

}  // namespace

namespace ecs::systems {

void SkyPresetSystem::tick(ecs::EntityRegistry& registry) const {
  registry.view<ecs::SkyComponent>([&](ecs::EntityId skyId, ecs::SkyComponent& sky) {
    (void)skyId;
    if (!sky.enabled) return;
    if (!sky.useSkyTypePreset) return;

    const SkyPreset p = presetFor(sky.skyType);
    applySkyPreset(sky, p);

    if (sky.linkedDirectionalLightEntity != ecs::kInvalidEntityId) {
      if (auto* light = registry.tryGet<ecs::LightComponent>(sky.linkedDirectionalLightEntity)) {
        applyLightPreset(*light, sky, p);
      }
    }

    if (sky.linkedFogVolumeEntity != ecs::kInvalidEntityId) {
      if (auto* fog = registry.tryGet<ecs::FogVolumeComponent>(sky.linkedFogVolumeEntity)) {
        applyFogPreset(*fog, p);
      }
    }
  });
}

}  // namespace ecs::systems

