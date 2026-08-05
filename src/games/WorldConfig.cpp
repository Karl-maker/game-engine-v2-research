#include "games/WorldConfig.h"

#include "data/JsonUtil.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/services/ChunkStreamingService.h"
#include "physics/LayerMask.h"

#include <fstream>
#include <sstream>

namespace games {

namespace {

bool readColor3(const data::JsonValue::Object& obj, const char* key, render::Color& out) {
  const data::JsonValue* v = data::getObjectKey(obj, key);
  if (!v) return false;
  math::Vec3 c{};
  if (!data::readVec3(*v, c)) return false;
  out.r = c.x;
  out.g = c.y;
  out.b = c.z;
  out.a = 1.0f;
  return true;
}

bool readColor4(const data::JsonValue::Object& obj, const char* key, render::Color& out) {
  const data::JsonValue* v = data::getObjectKey(obj, key);
  if (!v) return false;
  if (const auto* a = v->tryArray()) {
    if (a->size() < 4) return false;
    float r = out.r, g = out.g, b = out.b, aValue = out.a;
    if (!data::readFloat((*a)[0], r) || !data::readFloat((*a)[1], g) || !data::readFloat((*a)[2], b) ||
        !data::readFloat((*a)[3], aValue)) {
      return false;
    }
    out = {r, g, b, aValue};
    return true;
  }
  return readColor3(obj, key, out);
}

template <typename EnumT>
bool readEnumString(const data::JsonValue::Object& obj,
                    const char* key,
                    EnumT& out,
                    const std::initializer_list<std::pair<const char*, EnumT>>& map) {
  const data::JsonValue* v = data::getObjectKey(obj, key);
  if (!v) return false;
  std::string text;
  if (!data::readString(*v, text)) return false;
  for (const auto& [name, value] : map) {
    if (text == name) {
      out = value;
      return true;
    }
  }
  return false;
}

bool loadJsonRoot(const std::string& path, data::JsonValue::Object const*& outRoot, data::JsonValue& storage) {
  if (path.empty()) return false;
  std::ifstream f(path);
  if (!f.is_open()) return false;

  std::stringstream ss;
  ss << f.rdbuf();
  const std::string text = ss.str();
  if (text.empty()) return false;

  auto parsed = data::parseJson(text);
  if (!parsed.ok) return false;
  storage = std::move(parsed.value);
  outRoot = storage.tryObject();
  return outRoot != nullptr;
}

void applyPlayerDefaults(ecs::services::PlayableCharacterConfig& cfg) {
  cfg.base.transform.name = "business_man";
  cfg.base.transform.position = {0.0f, 50.0f, 0.0f};
  cfg.base.transform.rotationDeg = {0.0f, 0.0f, 0.0f};

  cfg.base.viewable.meshId = "business-man";
  cfg.base.viewable.meshKey = "assets/models/business-man/scene.gltf";
  cfg.base.viewable.meshType = ecs::MeshComponent::MeshType::Skinned;
  cfg.base.viewable.meshScale = {1.0f, 1.0f, 1.0f};
  cfg.base.viewable.skeletonId = "business-man#skin0";
  cfg.base.viewable.castShadows = true;
  cfg.base.viewable.receiveShadows = true;
  cfg.base.viewable.tags = {"character", "player"};
  cfg.base.viewable.shaderKey = "graphics/shaders/model";

  cfg.base.physical.hasRigidbody = true;
  cfg.base.physical.mass = 80.0f;
  cfg.base.physical.useGravity = true;
  cfg.base.physical.kinematic = false;
  cfg.base.physical.hasCollider = true;
  cfg.base.physical.colliderShape = ecs::ColliderComponent::Shape::Capsule;
  cfg.base.physical.colliderSize = {0.38f, 1.85f, 0.38f};
  cfg.base.physical.colliderOffset = {0.0f, 0.925f, 0.0f};
  cfg.base.physical.collisionLayer = physics::kLayerCharacter;

  cfg.base.stats.walkingSpeed = 1.8f;
  cfg.base.stats.runningSpeed = 7.0f;
  cfg.base.skeleton.skeletonData = "assets/models/business-man/scene.gltf";
  cfg.base.animation.enabled = true;
  cfg.base.pose.enabled = true;
  cfg.base.ik.enabled = true;
  cfg.base.sensorCone.enabled = true;

  cfg.camera.transform.name = "camera";
  cfg.camera.transform.position = {0.0f, 1.0f, -6.0f};
  cfg.camera.transform.rotationDeg = {12.0f, 0.0f, 0.0f};
  cfg.camera.height = 0.1f;

  cfg.hud.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
  cfg.hud.heightPx = 190.0f;
  cfg.hud.marginBottomPx = 43.0f;
  cfg.hud.fillHeightRatio = 0.15f;
  cfg.hud.fillWidthRatio = 0.67f;
  cfg.hud.fillOffsetPx = {50.0f, -4.0f};
  cfg.hud.flipU = true;
}

void applySkyDefaults(ecs::SkyComponent& sky) {
  sky.skyType = ecs::SkyComponent::SkyType::Day;
  sky.useSkyTypePreset = true;
  sky.cloudType = ecs::SkyComponent::CloudType::Scattered;
  sky.quality = ecs::SkyComponent::Quality::High;
  sky.cloudCoverage = 0.78f;
  sky.cloudDensity = 0.85f;
  sky.cloudScale = 1.0f;
  sky.cloudSpeed = 0.020f;
  sky.cloudWindDirection = {1.0f, 0.35f};
  sky.cloudTimeScale = 1.0f;
  sky.cloudTurbulence = 0.45f;
  sky.cloudLightAbsorption = 0.55f;
  sky.cloudHeightMeters = 220.0f;
  sky.starsSeed = 4242u;
  sky.starsIntensity = 2.1f;
  sky.starsDensity = 0.70f;
  sky.starsSize = 1.05f;
  sky.starsTwinkleStrength = 0.22f;
  sky.starsTwinkleSpeed = 0.55f;
}

void applyFogDefaults(ecs::FogVolumeComponent& fog, math::Vec3& anchor) {
  anchor = {0.0f, 4.0f, 0.0f};
  fog.density = 0.028f;
  fog.startDistance = 6.0f;
  fog.endDistance = 110.0f;
  fog.maxOpacity = 0.92f;
  fog.distanceExponent = 1.35f;
  fog.heightFalloff = 0.085f;
  fog.baseHeightOffset = -4.0f;
  fog.noiseScale = 0.028f;
  fog.noiseStrength = 0.42f;
  fog.detailNoiseScale = 0.095f;
  fog.detailNoiseStrength = 0.18f;
  fog.windDirection = {1.0f, 0.35f};
  fog.windSpeed = 0.75f;
  fog.enabled = true;
}

void applyRenderDefaults(ecs::RenderSettingsComponent& render) {
  render.enabled = true;
  render.shadowsEnabled = true;
  render.shadowQuality = 2;
  render.shadowStrength = 0.82f;
  render.shadowUseTessellation = true;
}

void applySunDefaults(ecs::LightComponent& sun) {
  sun.type = ecs::LightComponent::Type::Directional;
  sun.direction = math::normalize(math::Vec3{-0.32f, -1.0f, -0.18f});
  sun.intensity = 3.65f;
  sun.color = {1.0f, 0.96f, 0.88f};
  sun.castShadows = true;
  sun.shadowResolution = 2048u;
  sun.shadowBias = 0.0006f;
  sun.shadowDistance = 82.0f;
}

void applyChunkStreamingDefaults(ecs::services::ChunkStreamingConfig& chunkStreaming) {
  chunkStreaming.chunkSizeMeters = 96.0f;
  chunkStreaming.searchRadiusChunks = 3;
  chunkStreaming.loadProximityMeters = 140.0f;
  chunkStreaming.unloadProximityMeters = 196.0f;
  chunkStreaming.maxLoadsPerTick = 2;
  chunkStreaming.maxUnloadsPerTick = 4;
}

void readPlayerConfig(const data::JsonValue::Object& obj, ecs::services::PlayableCharacterConfig& cfg) {
  cfg.hasController = data::getBoolOr(obj, "hasController", cfg.hasController);

  if (const auto* name = data::getObjectKey(obj, "name")) data::readString(*name, cfg.base.transform.name);
  if (const auto* position = data::getObjectKey(obj, "position")) data::readVec3(*position, cfg.base.transform.position);
  if (const auto* rotation = data::getObjectKey(obj, "rotationDeg")) data::readVec3(*rotation, cfg.base.transform.rotationDeg);
  if (const auto* scale = data::getObjectKey(obj, "scale")) data::readVec3(*scale, cfg.base.transform.scale);

  if (const auto* meshKey = data::getObjectKey(obj, "meshKey")) data::readString(*meshKey, cfg.base.viewable.meshKey);
  if (const auto* meshId = data::getObjectKey(obj, "meshId")) data::readString(*meshId, cfg.base.viewable.meshId);
  if (const auto* skeletonId = data::getObjectKey(obj, "skeletonId")) data::readString(*skeletonId, cfg.base.viewable.skeletonId);
  if (const auto* skeletonData = data::getObjectKey(obj, "skeletonData")) data::readString(*skeletonData, cfg.base.skeleton.skeletonData);
  if (const auto* shaderKey = data::getObjectKey(obj, "shaderKey")) data::readString(*shaderKey, cfg.base.viewable.shaderKey);
  if (const auto* meshScale = data::getObjectKey(obj, "meshScale")) data::readVec3(*meshScale, cfg.base.viewable.meshScale);

  cfg.base.viewable.castShadows = data::getBoolOr(obj, "castShadows", cfg.base.viewable.castShadows);
  cfg.base.viewable.receiveShadows = data::getBoolOr(obj, "receiveShadows", cfg.base.viewable.receiveShadows);

  cfg.base.physical.mass = data::getFloatOr(obj, "mass", cfg.base.physical.mass);
  cfg.base.physical.useGravity = data::getBoolOr(obj, "useGravity", cfg.base.physical.useGravity);
  cfg.base.physical.kinematic = data::getBoolOr(obj, "kinematic", cfg.base.physical.kinematic);
  if (const auto* colliderSize = data::getObjectKey(obj, "colliderSize")) data::readVec3(*colliderSize, cfg.base.physical.colliderSize);
  if (const auto* colliderOffset = data::getObjectKey(obj, "colliderOffset")) data::readVec3(*colliderOffset, cfg.base.physical.colliderOffset);

  cfg.base.stats.walkingSpeed = data::getFloatOr(obj, "walkingSpeed", cfg.base.stats.walkingSpeed);
  cfg.base.stats.runningSpeed = data::getFloatOr(obj, "runningSpeed", cfg.base.stats.runningSpeed);

  cfg.base.animation.enabled = data::getBoolOr(obj, "animationEnabled", cfg.base.animation.enabled);
  cfg.base.pose.enabled = data::getBoolOr(obj, "poseEnabled", cfg.base.pose.enabled);
  cfg.base.ik.enabled = data::getBoolOr(obj, "ikEnabled", cfg.base.ik.enabled);
  cfg.base.sensorCone.enabled = data::getBoolOr(obj, "sensorConeEnabled", cfg.base.sensorCone.enabled);

  if (const auto* cameraObjV = data::getObjectKey(obj, "camera")) {
    if (const auto* cameraObj = cameraObjV->tryObject()) {
      if (const auto* position = data::getObjectKey(*cameraObj, "position")) data::readVec3(*position, cfg.camera.transform.position);
      if (const auto* rotation = data::getObjectKey(*cameraObj, "rotationDeg")) data::readVec3(*rotation, cfg.camera.transform.rotationDeg);
      if (const auto* targetOffset = data::getObjectKey(*cameraObj, "targetOffset")) data::readVec3(*targetOffset, cfg.camera.targetOffset);
      cfg.camera.distance = data::getFloatOr(*cameraObj, "distance", cfg.camera.distance);
      cfg.camera.height = data::getFloatOr(*cameraObj, "height", cfg.camera.height);
      cfg.camera.pitchDeg = data::getFloatOr(*cameraObj, "pitchDeg", cfg.camera.pitchDeg);
      cfg.camera.minPitchDeg = data::getFloatOr(*cameraObj, "minPitchDeg", cfg.camera.minPitchDeg);
      cfg.camera.maxPitchDeg = data::getFloatOr(*cameraObj, "maxPitchDeg", cfg.camera.maxPitchDeg);
      cfg.camera.depthOfFieldEnabled = data::getBoolOr(*cameraObj, "depthOfFieldEnabled", cfg.camera.depthOfFieldEnabled);
      cfg.camera.motionBlurEnabled = data::getBoolOr(*cameraObj, "motionBlurEnabled", cfg.camera.motionBlurEnabled);
    }
  }

  if (const auto* hudObjV = data::getObjectKey(obj, "hud")) {
    if (const auto* hudObj = hudObjV->tryObject()) {
      cfg.hud.enabled = data::getBoolOr(*hudObj, "enabled", cfg.hud.enabled);
      if (const auto* texturePath = data::getObjectKey(*hudObj, "texturePath")) data::readString(*texturePath, cfg.hud.texturePath);
      cfg.hud.heightPx = data::getFloatOr(*hudObj, "heightPx", cfg.hud.heightPx);
      cfg.hud.marginLeftPx = data::getFloatOr(*hudObj, "marginLeftPx", cfg.hud.marginLeftPx);
      cfg.hud.marginBottomPx = data::getFloatOr(*hudObj, "marginBottomPx", cfg.hud.marginBottomPx);
      cfg.hud.flipU = data::getBoolOr(*hudObj, "flipU", cfg.hud.flipU);
      cfg.hud.flipV = data::getBoolOr(*hudObj, "flipV", cfg.hud.flipV);
      cfg.hud.fillEnabled = data::getBoolOr(*hudObj, "fillEnabled", cfg.hud.fillEnabled);
      cfg.hud.fillWidthRatio = data::getFloatOr(*hudObj, "fillWidthRatio", cfg.hud.fillWidthRatio);
      cfg.hud.fillHeightRatio = data::getFloatOr(*hudObj, "fillHeightRatio", cfg.hud.fillHeightRatio);
      if (const auto* fillOffset = data::getObjectKey(*hudObj, "fillOffsetPx")) data::readVec2(*fillOffset, cfg.hud.fillOffsetPx);
      (void)readColor4(*hudObj, "fillColor", cfg.hud.fillColor);
      (void)readColor4(*hudObj, "tint", cfg.hud.tint);
    }
  }
}

void readSkyConfig(const data::JsonValue::Object& obj, ecs::SkyComponent& sky) {
  sky.enabled = data::getBoolOr(obj, "enabled", sky.enabled);
  sky.useSkyTypePreset = data::getBoolOr(obj, "useSkyTypePreset", sky.useSkyTypePreset);
  sky.cloudsEnabled = data::getBoolOr(obj, "cloudsEnabled", sky.cloudsEnabled);
  sky.sunEnabled = data::getBoolOr(obj, "sunEnabled", sky.sunEnabled);
  sky.starsEnabled = data::getBoolOr(obj, "starsEnabled", sky.starsEnabled);

  (void)readEnumString(obj,
                       "skyType",
                       sky.skyType,
                       {{"day", ecs::SkyComponent::SkyType::Day},
                        {"sunset", ecs::SkyComponent::SkyType::Sunset},
                        {"night", ecs::SkyComponent::SkyType::Night},
                        {"overcast", ecs::SkyComponent::SkyType::Overcast},
                        {"storm", ecs::SkyComponent::SkyType::Storm}});
  (void)readEnumString(obj,
                       "cloudType",
                       sky.cloudType,
                       {{"none", ecs::SkyComponent::CloudType::None},
                        {"wispy", ecs::SkyComponent::CloudType::Wispy},
                        {"scattered", ecs::SkyComponent::CloudType::Scattered},
                        {"broken", ecs::SkyComponent::CloudType::Broken},
                        {"overcast", ecs::SkyComponent::CloudType::Overcast},
                        {"storm", ecs::SkyComponent::CloudType::Storm}});
  (void)readEnumString(obj,
                       "quality",
                       sky.quality,
                       {{"low", ecs::SkyComponent::Quality::Low},
                        {"medium", ecs::SkyComponent::Quality::Medium},
                        {"high", ecs::SkyComponent::Quality::High},
                        {"ultra", ecs::SkyComponent::Quality::Ultra}});

  sky.cloudCoverage = data::getFloatOr(obj, "cloudCoverage", sky.cloudCoverage);
  sky.cloudDensity = data::getFloatOr(obj, "cloudDensity", sky.cloudDensity);
  sky.cloudScale = data::getFloatOr(obj, "cloudScale", sky.cloudScale);
  sky.cloudSpeed = data::getFloatOr(obj, "cloudSpeed", sky.cloudSpeed);
  sky.cloudTimeScale = data::getFloatOr(obj, "cloudTimeScale", sky.cloudTimeScale);
  sky.cloudTurbulence = data::getFloatOr(obj, "cloudTurbulence", sky.cloudTurbulence);
  sky.cloudLightAbsorption = data::getFloatOr(obj, "cloudLightAbsorption", sky.cloudLightAbsorption);
  sky.cloudHeightMeters = data::getFloatOr(obj, "cloudHeightMeters", sky.cloudHeightMeters);
  sky.starsIntensity = data::getFloatOr(obj, "starsIntensity", sky.starsIntensity);
  sky.starsDensity = data::getFloatOr(obj, "starsDensity", sky.starsDensity);
  sky.starsSize = data::getFloatOr(obj, "starsSize", sky.starsSize);
  sky.starsTwinkleStrength = data::getFloatOr(obj, "starsTwinkleStrength", sky.starsTwinkleStrength);
  sky.starsTwinkleSpeed = data::getFloatOr(obj, "starsTwinkleSpeed", sky.starsTwinkleSpeed);
  sky.starsSeed = static_cast<std::uint32_t>(data::getIntOr(obj, "starsSeed", static_cast<int>(sky.starsSeed)));

  if (const auto* wind = data::getObjectKey(obj, "cloudWindDirection")) data::readVec2(*wind, sky.cloudWindDirection);
  if (const auto* sunDir = data::getObjectKey(obj, "sunDirection")) data::readVec3(*sunDir, sky.sunDirection);
  (void)readColor4(obj, "sunTint", sky.sunTint);
}

void readFogConfig(const data::JsonValue::Object& obj, ecs::FogVolumeComponent& fog, math::Vec3& anchor) {
  fog.enabled = data::getBoolOr(obj, "enabled", fog.enabled);
  if (const auto* anchorV = data::getObjectKey(obj, "anchor")) data::readVec3(*anchorV, anchor);
  (void)readColor3(obj, "color", fog.color);
  fog.density = data::getFloatOr(obj, "density", fog.density);
  fog.startDistance = data::getFloatOr(obj, "startDistance", fog.startDistance);
  fog.endDistance = data::getFloatOr(obj, "endDistance", fog.endDistance);
  fog.maxOpacity = data::getFloatOr(obj, "maxOpacity", fog.maxOpacity);
  fog.distanceExponent = data::getFloatOr(obj, "distanceExponent", fog.distanceExponent);
  fog.heightFalloff = data::getFloatOr(obj, "heightFalloff", fog.heightFalloff);
  fog.baseHeightOffset = data::getFloatOr(obj, "baseHeightOffset", fog.baseHeightOffset);
  fog.horizonStrength = data::getFloatOr(obj, "horizonStrength", fog.horizonStrength);
  fog.noiseScale = data::getFloatOr(obj, "noiseScale", fog.noiseScale);
  fog.noiseStrength = data::getFloatOr(obj, "noiseStrength", fog.noiseStrength);
  fog.detailNoiseScale = data::getFloatOr(obj, "detailNoiseScale", fog.detailNoiseScale);
  fog.detailNoiseStrength = data::getFloatOr(obj, "detailNoiseStrength", fog.detailNoiseStrength);
  fog.windSpeed = data::getFloatOr(obj, "windSpeed", fog.windSpeed);
  if (const auto* windDir = data::getObjectKey(obj, "windDirection")) data::readVec2(*windDir, fog.windDirection);
}

void readRenderConfig(const data::JsonValue::Object& obj, ecs::RenderSettingsComponent& render) {
  render.enabled = data::getBoolOr(obj, "enabled", render.enabled);
  render.shadowsEnabled = data::getBoolOr(obj, "shadowsEnabled", render.shadowsEnabled);
  render.shadowQuality = data::getIntOr(obj, "shadowQuality", render.shadowQuality);
  render.shadowStrength = data::getFloatOr(obj, "shadowStrength", render.shadowStrength);
  render.shadowUseTessellation = data::getBoolOr(obj, "shadowUseTessellation", render.shadowUseTessellation);
  render.showRays = data::getBoolOr(obj, "showRays", render.showRays);
  render.showCollisionBoxes = data::getBoolOr(obj, "showCollisionBoxes", render.showCollisionBoxes);
  render.showCombatBoxes = data::getBoolOr(obj, "showCombatBoxes", render.showCombatBoxes);
  render.showSkeletonBones = data::getBoolOr(obj, "showSkeletonBones", render.showSkeletonBones);
}

void readSunConfig(const data::JsonValue::Object& obj, ecs::LightComponent& sun) {
  sun.enabled = data::getBoolOr(obj, "enabled", sun.enabled);
  sun.intensity = data::getFloatOr(obj, "intensity", sun.intensity);
  sun.castShadows = data::getBoolOr(obj, "castShadows", sun.castShadows);
  sun.shadowResolution = static_cast<std::uint32_t>(data::getIntOr(obj, "shadowResolution", static_cast<int>(sun.shadowResolution)));
  sun.shadowBias = data::getFloatOr(obj, "shadowBias", sun.shadowBias);
  sun.shadowDistance = data::getFloatOr(obj, "shadowDistance", sun.shadowDistance);
  if (const auto* direction = data::getObjectKey(obj, "direction")) data::readVec3(*direction, sun.direction);
  if (const auto* color = data::getObjectKey(obj, "color")) {
    math::Vec3 value{};
    if (data::readVec3(*color, value)) sun.color = value;
  }
}

void readChunkStreamingConfig(const data::JsonValue::Object& obj, ecs::services::ChunkStreamingConfig& chunkStreaming) {
  chunkStreaming.chunkSizeMeters = data::getFloatOr(obj, "chunkSizeMeters", chunkStreaming.chunkSizeMeters);
  chunkStreaming.searchRadiusChunks = std::max(0, data::getIntOr(obj, "searchRadiusChunks", chunkStreaming.searchRadiusChunks));
  chunkStreaming.loadProximityMeters = data::getFloatOr(obj, "loadProximityMeters", chunkStreaming.loadProximityMeters);
  chunkStreaming.unloadProximityMeters = data::getFloatOr(obj, "unloadProximityMeters", chunkStreaming.unloadProximityMeters);
  chunkStreaming.maxLoadsPerTick = std::max(0, data::getIntOr(obj, "maxLoadsPerTick", chunkStreaming.maxLoadsPerTick));
  chunkStreaming.maxUnloadsPerTick = std::max(0, data::getIntOr(obj, "maxUnloadsPerTick", chunkStreaming.maxUnloadsPerTick));
}

}  // namespace

bool loadPersistentWorldConfig(const std::string& chunkConfigPath, PersistentWorldConfig& outConfig) {
  outConfig = {};
  applyPlayerDefaults(outConfig.player);
  applySkyDefaults(outConfig.sky);
  applyFogDefaults(outConfig.fog, outConfig.fogAnchor);
  applyRenderDefaults(outConfig.renderSettings);
  applySunDefaults(outConfig.sun);
  applyChunkStreamingDefaults(outConfig.chunkStreaming);

  data::JsonValue storage;
  const data::JsonValue::Object* root = nullptr;
  if (!loadJsonRoot(chunkConfigPath, root, storage) || !root) return false;

  const auto* persistentV = data::getObjectKey(*root, "persistent");
  const auto* persistent = persistentV ? persistentV->tryObject() : nullptr;
  if (!persistent) return false;

  if (const auto* playerV = data::getObjectKey(*persistent, "player")) {
    if (const auto* player = playerV->tryObject()) {
      readPlayerConfig(*player, outConfig.player);
      outConfig.hasPlayer = true;
    }
  }

  if (const auto* skyV = data::getObjectKey(*persistent, "sky")) {
    if (const auto* sky = skyV->tryObject()) {
      readSkyConfig(*sky, outConfig.sky);
      outConfig.hasSky = true;
    }
  }

  if (const auto* fogV = data::getObjectKey(*persistent, "fog")) {
    if (const auto* fog = fogV->tryObject()) {
      readFogConfig(*fog, outConfig.fog, outConfig.fogAnchor);
      outConfig.hasFog = true;
    }
  }
  if (const auto* renderV = data::getObjectKey(*persistent, "render")) {
    if (const auto* render = renderV->tryObject()) {
      readRenderConfig(*render, outConfig.renderSettings);
      outConfig.hasRenderSettings = true;
    }
  }
  if (const auto* sunV = data::getObjectKey(*persistent, "sun")) {
    if (const auto* sun = sunV->tryObject()) {
      readSunConfig(*sun, outConfig.sun);
      outConfig.hasSun = true;
    }
  }
  if (const auto* chunkStreamingV = data::getObjectKey(*persistent, "chunkStreaming")) {
    if (const auto* chunkStreaming = chunkStreamingV->tryObject()) {
      readChunkStreamingConfig(*chunkStreaming, outConfig.chunkStreaming);
      outConfig.hasChunkStreaming = true;
    }
  }

  return outConfig.hasPlayer || outConfig.hasSky || outConfig.hasFog || outConfig.hasRenderSettings || outConfig.hasSun ||
         outConfig.hasChunkStreaming;
}

}  // namespace games
