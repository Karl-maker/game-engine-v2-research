#include "games/EnvironmentConfig.h"

#include "data/JsonUtil.h"

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

}  // namespace

bool loadEnvironmentFogConfig(const std::string& chunkConfigPath, ecs::FogVolumeComponent& fog) {
  if (chunkConfigPath.empty()) return false;

  std::ifstream f(chunkConfigPath);
  if (!f.is_open()) return false;

  std::stringstream ss;
  ss << f.rdbuf();
  const std::string text = ss.str();
  if (text.empty()) return false;

  auto parsed = data::parseJson(text);
  if (!parsed.ok) return false;

  const auto* root = parsed.value.tryObject();
  if (!root) return false;

  const auto* envV = data::getObjectKey(*root, "environment");
  const auto* env = envV ? envV->tryObject() : nullptr;
  if (!env) return false;

  const auto* fogV = data::getObjectKey(*env, "fog");
  const auto* fogObj = fogV ? fogV->tryObject() : nullptr;
  if (!fogObj) return false;

  fog.enabled = data::getBoolOr(*fogObj, "enabled", fog.enabled);
  (void)readColor3(*fogObj, "color", fog.color);
  fog.density = data::getFloatOr(*fogObj, "density", fog.density);
  fog.startDistance = data::getFloatOr(*fogObj, "startDistance", fog.startDistance);
  fog.endDistance = data::getFloatOr(*fogObj, "endDistance", fog.endDistance);
  fog.heightFalloff = data::getFloatOr(*fogObj, "heightFalloff", fog.heightFalloff);
  fog.baseHeightOffset = data::getFloatOr(
      *fogObj, "baseHeight", data::getFloatOr(*fogObj, "baseHeightOffset", fog.baseHeightOffset));

  const auto* underwaterV = data::getObjectKey(*fogObj, "underwater");
  const auto* underwater = underwaterV ? underwaterV->tryObject() : nullptr;
  if (underwater) {
    fog.underwaterEnabled = data::getBoolOr(*underwater, "enabled", fog.underwaterEnabled);
    (void)readColor3(*underwater, "color", fog.underwaterColor);
    fog.underwaterDensity = data::getFloatOr(*underwater, "density", fog.underwaterDensity);
    fog.underwaterStartDistance = data::getFloatOr(*underwater, "startDistance", fog.underwaterStartDistance);
    fog.underwaterEndDistance = data::getFloatOr(*underwater, "endDistance", fog.underwaterEndDistance);
    fog.underwaterBlendDepth = data::getFloatOr(*underwater, "blendDepth", fog.underwaterBlendDepth);
    fog.underwaterLevel = data::getFloatOr(*underwater, "waterLevel", fog.underwaterLevel);
  }

  return true;
}

}  // namespace games
