#include "ecs/factories/FactoryKeyService.h"

// Author: Karl-Johan Bailey

#include "data/JsonUtil.h"
#include "ecs/factories/ObjectFactory.h"
#include "ecs/factories/PhysicalObjectFactory.h"
#include "ecs/factories/ActorFactory.h"
#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/GrassPatchFactory.h"
#include "ecs/factories/TerrainFactory.h"
#include "ecs/factories/VfxFactory.h"
#include "ecs/factories/WeaponFactory.h"
#include "materials/presets/Presets.h"
#include "ecs/services/IEntityFactory.h"
#include "render/Color.h"
#include "math/Vec4.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ecs::services {

namespace {

math::Vec3 readWorldOrLocalPosition(
    const data::JsonValue::Object& obj,
    const FactoryContext& ctx) {
  math::Vec3 position{0.0f, 0.0f, 0.0f};
  if (const auto* v = data::getObjectKey(obj, "position")) (void)data::readVec3(*v, position);
  if (const auto* v = data::getObjectKey(obj, "positionLocal")) {
    math::Vec3 local{};
    if (data::readVec3(*v, local)) position = ctx.chunkOriginWorld + local;
  }
  return position;
}

ecs::MeshComponent::MeshType parseMeshType(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "skinned" || value == "skin") return ecs::MeshComponent::MeshType::Skinned;
  if (value == "procedural" || value == "proc") return ecs::MeshComponent::MeshType::Procedural;
  return ecs::MeshComponent::MeshType::Static;
}

terrain::NoiseType parseNoiseType(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "value") return terrain::NoiseType::Value;
  return terrain::NoiseType::Perlin;
}

bool readVec3OrUniform(
    const data::JsonValue& v,
    math::Vec3& out) {
  if (data::readVec3(v, out)) return true;
  float f = 0.0f;
  if (!data::readFloat(v, f)) return false;
  out = {f, f, f};
  return true;
}

std::vector<std::string> readStringArrayOrEmpty(const data::JsonValue& v) {
  std::vector<std::string> out;
  const auto* a = v.tryArray();
  if (!a) return out;
  out.reserve(a->size());
  for (const auto& el : *a) {
    std::string s;
    if (data::readString(el, s)) out.push_back(std::move(s));
  }
  return out;
}

physics::LayerMask parseLayerMask(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "world") return physics::kLayerWorld;
  if (value == "character" || value == "person") return physics::kLayerCharacter;
  return physics::kAllLayers;
}

ecs::ColliderComponent::Shape parseColliderShape(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "sphere") return ecs::ColliderComponent::Shape::Sphere;
  if (value == "capsule") return ecs::ColliderComponent::Shape::Capsule;
  if (value == "mesh") return ecs::ColliderComponent::Shape::Mesh;
  if (value == "terrain") return ecs::ColliderComponent::Shape::Terrain;
  return ecs::ColliderComponent::Shape::Box;
}

ecs::VfxComponent::Type parseVfxType(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "electricity" || value == "lightning" || value == "arc") return ecs::VfxComponent::Type::Electricity;
  if (value == "sparks" || value == "spark") return ecs::VfxComponent::Type::Sparks;
  if (value == "smoke") return ecs::VfxComponent::Type::Smoke;
  if (value == "steam") return ecs::VfxComponent::Type::Steam;
  return ecs::VfxComponent::Type::Fire;
}

ecs::VfxComponent::Quality parseVfxQuality(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "low") return ecs::VfxComponent::Quality::Low;
  if (value == "medium" || value == "med") return ecs::VfxComponent::Quality::Medium;
  if (value == "ultra") return ecs::VfxComponent::Quality::Ultra;
  return ecs::VfxComponent::Quality::High;
}

std::vector<render::TextureBinding> readTextureBindings(const data::JsonValue& v);
std::vector<render::MaterialParameter> readMaterialParameters(const data::JsonValue& v);
std::vector<ShaderBreakpointInput> readShaderBreakpoints(const data::JsonValue& v);

std::optional<ecs::ShaderComponent> resolveTerrainMaterialPreset(const std::string& preset) {
  if (preset == "Dirt") return materials::presets::Dirt();
  if (preset == "HighQualityDirt") return materials::presets::HighQualityDirt();
  if (preset == "HighQualityDirtRockLayer") return materials::presets::HighQualityDirtRockLayer();
  if (preset == "HighQualityDirtRockGrassLayer") return materials::presets::HighQualityDirtRockGrassLayer();
  if (preset == "Mulch") return materials::presets::Mulch();
  if (preset == "PebblyDirt") return materials::presets::PebblyDirt();
  if (preset == "Sand") return materials::presets::Sand();
  if (preset == "Stone") return materials::presets::Stone();
  if (preset == "StoneGrass") return materials::presets::StoneGrass();
  if (preset == "Sky") return materials::presets::SkyDay();
  if (preset == "RealisticSkyClouds") return materials::presets::RealisticSkyClouds();
  return std::nullopt;
}

std::optional<ecs::ShaderComponent> readTerrainMaterial(const data::JsonValue& v) {
  const auto* obj = v.tryObject();
  if (!obj) return std::nullopt;

  std::optional<ecs::ShaderComponent> shader;
  if (const auto* presetV = data::getObjectKey(*obj, "preset")) {
    std::string preset;
    if (data::readString(*presetV, preset)) {
      shader = resolveTerrainMaterialPreset(preset);
    }
  }
  if (!shader) shader.emplace();

  if (const auto* keyV = data::getObjectKey(*obj, "shaderKey")) {
    (void)data::readString(*keyV, shader->shader.key);
  }
  if (const auto* keyV = data::getObjectKey(*obj, "key")) {
    (void)data::readString(*keyV, shader->shader.key);
  }
  if (const auto* tx = data::getObjectKey(*obj, "textures")) shader->textures = readTextureBindings(*tx);
  if (const auto* pv = data::getObjectKey(*obj, "parameters")) shader->parameters = readMaterialParameters(*pv);
  if (const auto* lv = data::getObjectKey(*obj, "lodBreakpoints")) {
    const auto breakpoints = readShaderBreakpoints(*lv);
    shader->lodBreakpoints.reserve(shader->lodBreakpoints.size() + breakpoints.size());
    for (const auto& bp : breakpoints) {
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
      shader->lodBreakpoints.push_back(std::move(out));
    }
  }
  shader->castShadows = data::getBoolOr(*obj, "castShadows", shader->castShadows);
  shader->receiveShadows = data::getBoolOr(*obj, "receiveShadows", shader->receiveShadows);

  return shader;
}

render::TextureBinding readTextureBinding(const data::JsonValue::Object& obj) {
  render::TextureBinding out{};
  out.slot = data::getStringOr(obj, "slot", out.slot);
  out.srgb = data::getBoolOr(obj, "srgb", out.srgb);
  if (const auto* tex = data::getObjectKey(obj, "texture")) {
    if (const auto* to = tex->tryObject()) {
      out.texture.enabled = data::getBoolOr(*to, "enabled", out.texture.enabled);
      out.texture.key = data::getStringOr(*to, "key", out.texture.key);
      if (const auto* id = data::getObjectKey(*to, "id")) {
        int n = 0;
        if (data::readInt(*id, n) && n >= 0) out.texture.id = static_cast<std::uint32_t>(n);
      }
    } else {
      data::readString(*tex, out.texture.key);
    }
  }
  if (out.texture.key.empty()) {
    out.texture.key = data::getStringOr(obj, "key", out.texture.key);
    out.texture.enabled = data::getBoolOr(obj, "enabled", out.texture.enabled);
  }
  return out;
}

bool readMaterialValue(const data::JsonValue& v, render::MaterialParamValue& out) {
  if (const auto* b = v.tryBool()) {
    out = *b;
    return true;
  }
  if (const auto* n = v.tryNumber()) {
    const double d = *n;
    if (d >= static_cast<double>(std::numeric_limits<int>::min()) && d <= static_cast<double>(std::numeric_limits<int>::max()) &&
        std::floor(d) == d) {
      out = static_cast<int>(d);
    } else {
      out = static_cast<float>(d);
    }
    return true;
  }
  if (const auto* a = v.tryArray()) {
    if (a->size() == 2) {
      float x = 0.0f, y = 0.0f;
      if (!data::readFloat((*a)[0], x) || !data::readFloat((*a)[1], y)) return false;
      out = math::Vec2{x, y};
      return true;
    }
    if (a->size() == 3) {
      float x = 0.0f, y = 0.0f, z = 0.0f;
      if (!data::readFloat((*a)[0], x) || !data::readFloat((*a)[1], y) || !data::readFloat((*a)[2], z)) return false;
      out = math::Vec3{x, y, z};
      return true;
    }
    if (a->size() == 4) {
      float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
      if (!data::readFloat((*a)[0], x) || !data::readFloat((*a)[1], y) || !data::readFloat((*a)[2], z) ||
          !data::readFloat((*a)[3], w)) {
        return false;
      }
      out = math::Vec4{x, y, z, w};
      return true;
    }
  }
  if (const auto* o = v.tryObject()) {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    if (const auto* rv = data::getObjectKey(*o, "r")) (void)data::readFloat(*rv, r);
    if (const auto* gv = data::getObjectKey(*o, "g")) (void)data::readFloat(*gv, g);
    if (const auto* bv = data::getObjectKey(*o, "b")) (void)data::readFloat(*bv, b);
    if (const auto* av = data::getObjectKey(*o, "a")) (void)data::readFloat(*av, a);
    out = render::Color{r, g, b, a};
    return true;
  }
  return false;
}

render::MaterialParameter readMaterialParameter(const data::JsonValue::Object& obj) {
  render::MaterialParameter out{};
  out.name = data::getStringOr(obj, "name", out.name);
  if (const auto* v = data::getObjectKey(obj, "value")) {
    (void)readMaterialValue(*v, out.value);
  }
  return out;
}

std::vector<render::TextureBinding> readTextureBindings(const data::JsonValue& v) {
  std::vector<render::TextureBinding> out;
  if (const auto* arr = v.tryArray()) {
    out.reserve(arr->size());
    for (const auto& el : *arr) {
      if (const auto* obj = el.tryObject()) out.push_back(readTextureBinding(*obj));
    }
  }
  return out;
}

std::vector<render::MaterialParameter> readMaterialParameters(const data::JsonValue& v) {
  std::vector<render::MaterialParameter> out;
  if (const auto* arr = v.tryArray()) {
    out.reserve(arr->size());
    for (const auto& el : *arr) {
      if (const auto* obj = el.tryObject()) out.push_back(readMaterialParameter(*obj));
    }
  }
  return out;
}

std::vector<ShaderBreakpointInput> readShaderBreakpoints(const data::JsonValue& v) {
  std::vector<ShaderBreakpointInput> out;
  if (const auto* arr = v.tryArray()) {
    out.reserve(arr->size());
    for (const auto& el : *arr) {
      if (const auto* obj = el.tryObject()) {
        ShaderBreakpointInput bp{};
        bp.distanceMeters = data::getFloatOr(*obj, "distanceMeters", bp.distanceMeters);
        if (const auto* tex = data::getObjectKey(*obj, "textures")) bp.textures = readTextureBindings(*tex);
        if (const auto* params = data::getObjectKey(*obj, "parameters")) bp.parameters = readMaterialParameters(*params);
        bp.overrideTessellation = data::getBoolOr(*obj, "overrideTessellation", bp.overrideTessellation);
        bp.tessNear = data::getFloatOr(*obj, "tessNear", bp.tessNear);
        bp.tessFar = data::getFloatOr(*obj, "tessFar", bp.tessFar);
        bp.tessMin = data::getFloatOr(*obj, "tessMin", bp.tessMin);
        bp.tessMax = data::getFloatOr(*obj, "tessMax", bp.tessMax);
        bp.tessQuality = data::getIntOr(*obj, "tessQuality", bp.tessQuality);
        out.push_back(std::move(bp));
      }
    }
  }
  return out;
}

bool readColorValue(const data::JsonValue& v, render::Color& out) {
  render::MaterialParamValue value;
  if (!readMaterialValue(v, value)) return false;
  if (const auto* c = std::get_if<render::Color>(&value)) {
    out = *c;
    return true;
  }
  if (const auto* v4 = std::get_if<math::Vec4>(&value)) {
    out = {v4->x, v4->y, v4->z, v4->w};
    return true;
  }
  if (const auto* v3 = std::get_if<math::Vec3>(&value)) {
    out = {v3->x, v3->y, v3->z, 1.0f};
    return true;
  }
  return false;
}

TransformInput readTransformInput(const data::JsonValue::Object& obj, const FactoryContext& ctx) {
  TransformInput t{};
  t.name = data::getStringOr(obj, "name", t.name);
  t.position = readWorldOrLocalPosition(obj, ctx);
  if (const auto* v = data::getObjectKey(obj, "rotationDeg")) (void)data::readVec3(*v, t.rotationDeg);
  if (const auto* v = data::getObjectKey(obj, "scale")) (void)readVec3OrUniform(*v, t.scale);
  return t;
}

ViewableInput readViewableInput(const data::JsonValue::Object& obj) {
  ViewableInput v{};
  v.meshId = data::getStringOr(obj, "meshId", v.meshId);
  if (const auto* k = data::getObjectKey(obj, "meshKey")) (void)data::readString(*k, v.meshKey);
  if (const auto* t = data::getObjectKey(obj, "meshType")) {
    std::string s;
    if (data::readString(*t, s)) v.meshType = parseMeshType(std::move(s));
  }
  if (const auto* s = data::getObjectKey(obj, "meshScale")) (void)readVec3OrUniform(*s, v.meshScale);
  v.skeletonId = data::getStringOr(obj, "skeletonId", v.skeletonId);
  v.visible = data::getBoolOr(obj, "visible", v.visible);
  v.castShadows = data::getBoolOr(obj, "castShadows", v.castShadows);
  v.receiveShadows = data::getBoolOr(obj, "receiveShadows", v.receiveShadows);
  if (const auto* tv = data::getObjectKey(obj, "tags")) v.tags = readStringArrayOrEmpty(*tv);
  if (const auto* sk = data::getObjectKey(obj, "shaderKey")) (void)data::readString(*sk, v.shaderKey);
  if (const auto* tx = data::getObjectKey(obj, "textures")) v.textures = readTextureBindings(*tx);
  if (const auto* pv = data::getObjectKey(obj, "parameters")) v.parameters = readMaterialParameters(*pv);
  if (const auto* lv = data::getObjectKey(obj, "lodBreakpoints")) v.lodBreakpoints = readShaderBreakpoints(*lv);

  // Optional nested objects.
  if (const auto* mv = data::getObjectKey(obj, "mesh")) {
    if (const auto* mo = mv->tryObject()) {
      v.meshKey = data::getStringOr(*mo, "key", v.meshKey);
      v.meshId = data::getStringOr(*mo, "id", v.meshId);
      if (const auto* t = data::getObjectKey(*mo, "type")) {
        std::string s;
        if (data::readString(*t, s)) v.meshType = parseMeshType(std::move(s));
      }
      if (const auto* s = data::getObjectKey(*mo, "scale")) (void)readVec3OrUniform(*s, v.meshScale);
      v.skeletonId = data::getStringOr(*mo, "skeletonId", v.skeletonId);
      v.visible = data::getBoolOr(*mo, "visible", v.visible);
      v.castShadows = data::getBoolOr(*mo, "castShadows", v.castShadows);
      v.receiveShadows = data::getBoolOr(*mo, "receiveShadows", v.receiveShadows);
      if (const auto* tv = data::getObjectKey(*mo, "tags")) v.tags = readStringArrayOrEmpty(*tv);
    }
  }
  if (const auto* sv = data::getObjectKey(obj, "shader")) {
    if (const auto* so = sv->tryObject()) {
      v.shaderKey = data::getStringOr(*so, "key", v.shaderKey);
      if (const auto* tx = data::getObjectKey(*so, "textures")) v.textures = readTextureBindings(*tx);
      if (const auto* pv = data::getObjectKey(*so, "parameters")) v.parameters = readMaterialParameters(*pv);
      if (const auto* lv = data::getObjectKey(*so, "lodBreakpoints")) v.lodBreakpoints = readShaderBreakpoints(*lv);
      v.castShadows = data::getBoolOr(*so, "castShadows", v.castShadows);
      v.receiveShadows = data::getBoolOr(*so, "receiveShadows", v.receiveShadows);
    }
  }

  return v;
}

TerrainConfig readTerrainInput(const data::JsonValue::Object& obj, const FactoryContext& ctx) {
  TerrainConfig t{};
  t.transform = readTransformInput(obj, ctx);
  t.transform.name = data::getStringOr(obj, "name", t.transform.name);
  t.gridWidth = data::getIntOr(obj, "gridWidth", t.gridWidth);
  t.gridHeight = data::getIntOr(obj, "gridHeight", t.gridHeight);
  t.cellSizeMeters = data::getFloatOr(obj, "cellSizeMeters", t.cellSizeMeters);
  t.heightScaleMeters = data::getFloatOr(obj, "heightScaleMeters", t.heightScaleMeters);
  if (const auto* v = data::getObjectKey(obj, "noiseSeed")) {
    int seed = static_cast<int>(t.noiseSeed);
    if (data::readInt(*v, seed) && seed >= 0) t.noiseSeed = static_cast<std::uint32_t>(seed);
  }
  if (const auto* nv = data::getObjectKey(obj, "noise")) {
    if (const auto* no = nv->tryObject()) {
      if (const auto* seedV = data::getObjectKey(*no, "seed")) {
        int seed = static_cast<int>(t.noise.seed);
        if (data::readInt(*seedV, seed) && seed >= 0) t.noise.seed = static_cast<std::uint32_t>(seed);
      }
      t.noise.frequency = data::getFloatOr(*no, "frequency", t.noise.frequency);
      t.noise.octaves = data::getIntOr(*no, "octaves", t.noise.octaves);
      t.noise.lacunarity = data::getFloatOr(*no, "lacunarity", t.noise.lacunarity);
      t.noise.persistence = data::getFloatOr(*no, "persistence", t.noise.persistence);
    }
  }
  t.lodMaxRenderDistance = data::getFloatOr(obj, "lodMaxRenderDistance", t.lodMaxRenderDistance);
  t.lodStep1Distance = data::getFloatOr(obj, "lodStep1Distance", t.lodStep1Distance);
  t.lodStep2Distance = data::getFloatOr(obj, "lodStep2Distance", t.lodStep2Distance);
  t.lodStep4Distance = data::getFloatOr(obj, "lodStep4Distance", t.lodStep4Distance);
  t.lodStep8Distance = data::getFloatOr(obj, "lodStep8Distance", t.lodStep8Distance);
  t.lodStep16Distance = data::getFloatOr(obj, "lodStep16Distance", t.lodStep16Distance);
  t.lodForceNearDistance = data::getFloatOr(obj, "lodForceNearDistance", t.lodForceNearDistance);
  t.tessLockDistance = data::getFloatOr(obj, "tessLockDistance", t.tessLockDistance);
  t.tessEnableDistance = data::getFloatOr(obj, "tessEnableDistance", t.tessEnableDistance);
  t.tessDisableDistance = data::getFloatOr(obj, "tessDisableDistance", t.tessDisableDistance);
  t.viewDotBias = data::getFloatOr(obj, "viewDotBias", t.viewDotBias);
  t.hasCollider = data::getBoolOr(obj, "colliderEnabled", t.hasCollider);
  t.colliderThicknessMeters = data::getFloatOr(obj, "colliderThicknessMeters", t.colliderThicknessMeters);
  t.hasShader = data::getBoolOr(obj, "shaderEnabled", t.hasShader);
  t.shaderKey = data::getStringOr(obj, "shaderKey", t.shaderKey);

  if (const auto* mv = data::getObjectKey(obj, "material")) {
    t.material = readTerrainMaterial(*mv);
  }
  if (const auto* tx = data::getObjectKey(obj, "textures")) t.viewable.textures = readTextureBindings(*tx);
  if (const auto* pv = data::getObjectKey(obj, "parameters")) t.viewable.parameters = readMaterialParameters(*pv);
  if (const auto* lv = data::getObjectKey(obj, "lodBreakpoints")) t.viewable.lodBreakpoints = readShaderBreakpoints(*lv);
  t.viewable.visible = data::getBoolOr(obj, "visible", t.viewable.visible);
  t.viewable.castShadows = data::getBoolOr(obj, "castShadows", t.viewable.castShadows);
  t.viewable.receiveShadows = data::getBoolOr(obj, "receiveShadows", t.viewable.receiveShadows);

  return t;
}

GrassInput readGrassInput(const data::JsonValue::Object& obj) {
  GrassInput g{};

  auto readNoise = [&](const data::JsonValue& v, terrain::NoiseConfig& out) {
    const auto* no = v.tryObject();
    if (!no) return;
    if (const auto* tv = data::getObjectKey(*no, "type")) {
      std::string s;
      if (data::readString(*tv, s)) out.type = parseNoiseType(std::move(s));
    }
    if (const auto* seedV = data::getObjectKey(*no, "seed")) {
      int seed = static_cast<int>(out.seed);
      if (data::readInt(*seedV, seed) && seed >= 0) out.seed = static_cast<std::uint32_t>(seed);
    }
    out.frequency = data::getFloatOr(*no, "frequency", out.frequency);
    out.octaves = data::getIntOr(*no, "octaves", out.octaves);
    out.lacunarity = data::getFloatOr(*no, "lacunarity", out.lacunarity);
    out.persistence = data::getFloatOr(*no, "persistence", out.persistence);
  };

  auto readLayer = [&](const data::JsonValue& v) {
    const auto* lo = v.tryObject();
    if (!lo) return;
    GrassLayerInput l{};
    l.species = data::getStringOr(*lo, "species", l.species);
    l.description = data::getStringOr(*lo, "description", l.description);
    l.density = data::getFloatOr(*lo, "density", l.density);
    l.minScale = data::getFloatOr(*lo, "minScale", l.minScale);
    l.maxScale = data::getFloatOr(*lo, "maxScale", l.maxScale);
    l.bladeSpacing = data::getFloatOr(*lo, "bladeSpacing", l.bladeSpacing);
    l.bendStrength = data::getFloatOr(*lo, "bendStrength", l.bendStrength);
    l.curveStrength = data::getFloatOr(*lo, "curveStrength", l.curveStrength);
    l.twistStrength = data::getFloatOr(*lo, "twistStrength", l.twistStrength);
    l.minSlopeDeg = data::getFloatOr(*lo, "minSlopeDeg", l.minSlopeDeg);
    l.maxSlopeDeg = data::getFloatOr(*lo, "maxSlopeDeg", l.maxSlopeDeg);
    l.minAltitude = data::getFloatOr(*lo, "minAltitude", l.minAltitude);
    l.maxAltitude = data::getFloatOr(*lo, "maxAltitude", l.maxAltitude);
    l.noiseScale = data::getFloatOr(*lo, "noiseScale", l.noiseScale);
    l.noiseStrength = data::getFloatOr(*lo, "noiseStrength", l.noiseStrength);
    l.windStrength = data::getFloatOr(*lo, "windStrength", l.windStrength);
    l.maxDistance = data::getFloatOr(*lo, "maxDistance", l.maxDistance);
    g.layers.push_back(std::move(l));
  };

  auto readShaderInto = [&](const data::JsonValue::Object& src) {
    if (const auto* k = data::getObjectKey(src, "shaderKey")) (void)data::readString(*k, g.shaderKey);
    g.shaderKey = data::getStringOr(src, "key", g.shaderKey);
    if (const auto* tx = data::getObjectKey(src, "textures")) g.textures = readTextureBindings(*tx);
    if (const auto* pv = data::getObjectKey(src, "parameters")) g.parameters = readMaterialParameters(*pv);
    if (const auto* lv = data::getObjectKey(src, "lodBreakpoints")) g.lodBreakpoints = readShaderBreakpoints(*lv);
    g.doubleSided = data::getBoolOr(src, "doubleSided", g.doubleSided);
    g.depthWrite = data::getBoolOr(src, "depthWrite", g.depthWrite);
    g.castShadows = data::getBoolOr(src, "castShadows", g.castShadows);
    g.receiveShadows = data::getBoolOr(src, "receiveShadows", g.receiveShadows);
  };

  auto readInto = [&](const data::JsonValue::Object& src) {
    g.enabled = data::getBoolOr(src, "enabled", g.enabled);
    if (const auto* a = data::getObjectKey(src, "area")) (void)data::readVec3(*a, g.area);
    g.densityMultiplier = data::getFloatOr(src, "densityMultiplier", g.densityMultiplier);
    if (const auto* seedV = data::getObjectKey(src, "seed")) {
      int seed = static_cast<int>(g.seed);
      if (data::readInt(*seedV, seed) && seed >= 0) g.seed = static_cast<std::uint32_t>(seed);
    }

    if (const auto* dv = data::getObjectKey(src, "densityNoise")) readNoise(*dv, g.densityNoise);
    g.densityNoiseThreshold = data::getFloatOr(src, "densityNoiseThreshold", g.densityNoiseThreshold);
    g.densityNoiseContrast = data::getFloatOr(src, "densityNoiseContrast", g.densityNoiseContrast);
    g.densityNoiseStrength = data::getFloatOr(src, "densityNoiseStrength", g.densityNoiseStrength);

    if (const auto* iv = data::getObjectKey(src, "islandNoise")) readNoise(*iv, g.islandNoise);
    if (const auto* ov = data::getObjectKey(src, "islandNoiseOffset")) (void)data::readVec3(*ov, g.islandNoiseOffset);
    g.islandNoiseThreshold = data::getFloatOr(src, "islandNoiseThreshold", g.islandNoiseThreshold);
    g.islandNoiseSoftness = data::getFloatOr(src, "islandNoiseSoftness", g.islandNoiseSoftness);
    g.islandNoiseContrast = data::getFloatOr(src, "islandNoiseContrast", g.islandNoiseContrast);
    g.islandNoiseStrength = data::getFloatOr(src, "islandNoiseStrength", g.islandNoiseStrength);

    if (const auto* tv = data::getObjectKey(src, "sourceTerrainEntity")) {
      int n = 0;
      if (data::readInt(*tv, n) && n > 0) g.sourceTerrainEntity = static_cast<ecs::EntityId>(n);
    }
    if (const auto* tv = data::getObjectKey(src, "sourceTerrain")) {
      if (const auto* n = tv->tryNumber()) {
        if (*n > 0) g.sourceTerrainEntity = static_cast<ecs::EntityId>(static_cast<std::uint32_t>(*n));
      } else if (const auto* to = tv->tryObject()) {
        g.autoBindTerrain = data::getBoolOr(*to, "autoBind", g.autoBindTerrain);
        if (const auto* ev = data::getObjectKey(*to, "entity")) {
          int n = 0;
          if (data::readInt(*ev, n) && n > 0) g.sourceTerrainEntity = static_cast<ecs::EntityId>(n);
        }
      } else if (const auto* b = tv->tryBool()) {
        g.autoBindTerrain = *b;
      } else if (const auto* s = tv->tryString()) {
        g.autoBindTerrain = (*s == "auto");
      }
    }
    g.autoBindTerrain = data::getBoolOr(src, "autoBindTerrain", g.autoBindTerrain);

    if (const auto* lv = data::getObjectKey(src, "layers")) {
      if (const auto* la = lv->tryArray()) {
        g.layers.clear();
        g.layers.reserve(la->size());
        for (const auto& el : *la) readLayer(el);
      }
    }

    g.interactionEnabled = data::getBoolOr(src, "interactionEnabled", g.interactionEnabled);
    g.interactionRadiusMeters = data::getFloatOr(src, "interactionRadiusMeters", g.interactionRadiusMeters);
    g.interactionStrength = data::getFloatOr(src, "interactionStrength", g.interactionStrength);

    g.castShadows = data::getBoolOr(src, "castShadows", g.castShadows);
    g.receiveShadows = data::getBoolOr(src, "receiveShadows", g.receiveShadows);
    g.lodBias = data::getFloatOr(src, "lodBias", g.lodBias);

    g.hasShader = data::getBoolOr(src, "shaderEnabled", g.hasShader);
    if (const auto* sk = data::getObjectKey(src, "shader")) {
      if (const auto* so = sk->tryObject()) readShaderInto(*so);
    }
    if (const auto* tx = data::getObjectKey(src, "textures")) g.textures = readTextureBindings(*tx);
    if (const auto* pv = data::getObjectKey(src, "parameters")) g.parameters = readMaterialParameters(*pv);
    if (const auto* lv = data::getObjectKey(src, "lodBreakpoints")) g.lodBreakpoints = readShaderBreakpoints(*lv);
    if (const auto* k = data::getObjectKey(src, "shaderKey")) (void)data::readString(*k, g.shaderKey);
    g.shaderKey = data::getStringOr(src, "shaderKey", g.shaderKey);
  };

  if (const auto* gv = data::getObjectKey(obj, "grass")) {
    if (const auto* go = gv->tryObject()) {
      readInto(*go);
    }
  }

  readInto(obj);
  return g;
}

VfxConfig readVfxInput(const data::JsonValue::Object& obj, const FactoryContext& ctx) {
  VfxConfig v{};
  v.transform = readTransformInput(obj, ctx);
  v.transform.name = data::getStringOr(obj, "name", v.transform.name);

  auto readInto = [&](const data::JsonValue::Object& src) {
    if (const auto* t = data::getObjectKey(src, "type")) {
      std::string s;
      if (data::readString(*t, s)) v.vfx.type = parseVfxType(std::move(s));
    }
    if (const auto* q = data::getObjectKey(src, "quality")) {
      std::string s;
      if (data::readString(*q, s)) v.vfx.quality = parseVfxQuality(std::move(s));
    }
    v.vfx.enabled = data::getBoolOr(src, "enabled", v.vfx.enabled);
    v.vfx.autoQuality = data::getBoolOr(src, "autoQuality", v.vfx.autoQuality);
    v.vfx.maxRenderDistance = data::getFloatOr(src, "maxRenderDistance", v.vfx.maxRenderDistance);
    v.vfx.lodNearDistance = data::getFloatOr(src, "lodNearDistance", v.vfx.lodNearDistance);
    v.vfx.lodMidDistance = data::getFloatOr(src, "lodMidDistance", v.vfx.lodMidDistance);
    v.vfx.lodFarDistance = data::getFloatOr(src, "lodFarDistance", v.vfx.lodFarDistance);
    v.vfx.lodUltraDistance = data::getFloatOr(src, "lodUltraDistance", v.vfx.lodUltraDistance);
    v.vfx.lodForceNearDistance = data::getFloatOr(src, "lodForceNearDistance", v.vfx.lodForceNearDistance);
    v.vfx.viewDotBias = data::getFloatOr(src, "viewDotBias", v.vfx.viewDotBias);
    v.vfx.intensity = data::getFloatOr(src, "intensity", v.vfx.intensity);
    v.vfx.spawnRate = data::getFloatOr(src, "spawnRate", v.vfx.spawnRate);
    v.vfx.burstInterval = data::getFloatOr(src, "burstInterval", v.vfx.burstInterval);
    v.vfx.lifetimeSeconds = data::getFloatOr(src, "lifetimeSeconds", v.vfx.lifetimeSeconds);
    v.vfx.sizeMeters = data::getFloatOr(src, "sizeMeters", v.vfx.sizeMeters);
    v.vfx.sizeVariance = data::getFloatOr(src, "sizeVariance", v.vfx.sizeVariance);
    v.vfx.speedMetersPerSecond = data::getFloatOr(src, "speedMetersPerSecond", v.vfx.speedMetersPerSecond);
    v.vfx.speedVariance = data::getFloatOr(src, "speedVariance", v.vfx.speedVariance);
    v.vfx.gravityScale = data::getFloatOr(src, "gravityScale", v.vfx.gravityScale);
    v.vfx.drag = data::getFloatOr(src, "drag", v.vfx.drag);
    v.vfx.flickerStrength = data::getFloatOr(src, "flickerStrength", v.vfx.flickerStrength);
    v.vfx.flickerSpeed = data::getFloatOr(src, "flickerSpeed", v.vfx.flickerSpeed);
    v.vfx.looping = data::getBoolOr(src, "looping", v.vfx.looping);
    v.vfx.castLight = data::getBoolOr(src, "castLight", v.vfx.castLight);
    if (const auto* seedV = data::getObjectKey(src, "seed")) {
      int seed = static_cast<int>(v.vfx.seed);
      if (data::readInt(*seedV, seed) && seed >= 0) v.vfx.seed = static_cast<std::uint32_t>(seed);
    }
    v.vfx.heightMeters = data::getFloatOr(src, "heightMeters", v.vfx.heightMeters);
    v.vfx.upwardBias = data::getFloatOr(src, "upwardBias", v.vfx.upwardBias);
    v.vfx.spreadRadiusMeters = data::getFloatOr(src, "spreadRadiusMeters", v.vfx.spreadRadiusMeters);
    v.vfx.heatHazeStrength = data::getFloatOr(src, "heatHazeStrength", v.vfx.heatHazeStrength);
    v.vfx.chargeLengthMeters = data::getFloatOr(src, "chargeLengthMeters", v.vfx.chargeLengthMeters);
    v.vfx.arcJitter = data::getFloatOr(src, "arcJitter", v.vfx.arcJitter);
    v.vfx.branchCount = data::getIntOr(src, "branchCount", v.vfx.branchCount);
    v.vfx.segmentCount = data::getIntOr(src, "segmentCount", v.vfx.segmentCount);
    v.vfx.pulseSpeed = data::getFloatOr(src, "pulseSpeed", v.vfx.pulseSpeed);
    v.vfx.sparkCount = data::getIntOr(src, "sparkCount", v.vfx.sparkCount);
    v.vfx.sparkSpreadDegrees = data::getFloatOr(src, "sparkSpreadDegrees", v.vfx.sparkSpreadDegrees);
    v.vfx.sparkTrailLengthMeters = data::getFloatOr(src, "sparkTrailLengthMeters", v.vfx.sparkTrailLengthMeters);
    v.vfx.sparkFadeSeconds = data::getFloatOr(src, "sparkFadeSeconds", v.vfx.sparkFadeSeconds);
    if (const auto* c = data::getObjectKey(src, "primaryColor")) (void)readColorValue(*c, v.vfx.primaryColor);
    if (const auto* c = data::getObjectKey(src, "secondaryColor")) (void)readColorValue(*c, v.vfx.secondaryColor);
  };

  if (const auto* component = data::getObjectKey(obj, "component")) {
    if (const auto* co = component->tryObject()) {
      readInto(*co);
    }
  }
  readInto(obj);

  return v;
}

SkeletonInput readSkeletonInput(const data::JsonValue::Object& obj) {
  SkeletonInput s{};
  if (const auto* v = data::getObjectKey(obj, "skeletonData")) {
    (void)data::readString(*v, s.skeletonData);
  }
  if (const auto* sv = data::getObjectKey(obj, "skeleton")) {
    if (const auto* so = sv->tryObject()) {
      s.skeletonData = data::getStringOr(*so, "data", s.skeletonData);
    }
  }
  return s;
}

StatsInput readStatsInput(const data::JsonValue::Object& obj) {
  StatsInput stats{};

  if (const auto* sv = data::getObjectKey(obj, "stats")) {
    if (const auto* so = sv->tryObject()) {
      stats.walkingSpeed =
          data::getFloatOr(*so, "walkingSpeed", stats.walkingSpeed);

      stats.runningSpeed =
          data::getFloatOr(*so, "runningSpeed", stats.runningSpeed);

      stats.baseAttack =
          data::getFloatOr(*so, "baseAttack", stats.baseAttack);

      stats.defense =
          data::getFloatOr(*so, "defense", stats.defense);

      stats.health =
          data::getFloatOr(*so, "health", stats.health);

      stats.maxHealth =
          data::getFloatOr(*so, "maxHealth", stats.maxHealth);

      stats.specialAttack =
          data::getFloatOr(*so, "specialAttack", stats.specialAttack);

      stats.speed =
          data::getFloatOr(*so, "speed", stats.speed);

      stats.swimmingSpeed =
          data::getFloatOr(*so, "swimmingSpeed", stats.swimmingSpeed);
    }
  }

  return stats;
}

PhysicalInput readPhysicalInput(const data::JsonValue::Object& obj) {
  PhysicalInput p{};
  if (const auto* pv = data::getObjectKey(obj, "physical")) {
    if (const auto* po = pv->tryObject()) {
      p.hasRigidbody = data::getBoolOr(*po, "hasRigidbody", p.hasRigidbody);
      p.mass = data::getFloatOr(*po, "mass", p.mass);
      p.useGravity = data::getBoolOr(*po, "useGravity", p.useGravity);
      p.kinematic = data::getBoolOr(*po, "kinematic", p.kinematic);

      p.hasCollider = data::getBoolOr(*po, "hasCollider", p.hasCollider);
      p.colliderIsTrigger = data::getBoolOr(*po, "isTrigger", p.colliderIsTrigger);
      if (const auto* sv = data::getObjectKey(*po, "shape")) {
        std::string s;
        if (data::readString(*sv, s)) p.colliderShape = parseColliderShape(std::move(s));
      }
      if (const auto* sv = data::getObjectKey(*po, "size")) (void)readVec3OrUniform(*sv, p.colliderSize);
      if (const auto* ov = data::getObjectKey(*po, "offset")) (void)data::readVec3(*ov, p.colliderOffset);
      if (const auto* lv = data::getObjectKey(*po, "layer")) {
        if (const auto* n = lv->tryNumber()) {
          p.collisionLayer = static_cast<physics::LayerMask>(static_cast<std::uint32_t>(*n));
        } else {
          std::string s;
          if (data::readString(*lv, s)) p.collisionLayer = parseLayerMask(std::move(s));
        }
      }
    }
  }
  return p;
}

class ObjectJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    ObjectConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.transform = readTransformInput(*obj, ctx);
      cfg.transform.name = data::getStringOr(*obj, "name", cfg.transform.name);
      cfg.viewable = readViewableInput(*obj);
    }

    ObjectFactory factory;
    return factory.create(registry, cfg);
  }
};

class PhysicalObjectJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    PhysicalObjectConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.transform = readTransformInput(*obj, ctx);
      cfg.transform.name = data::getStringOr(*obj, "name", cfg.transform.name);
      cfg.viewable = readViewableInput(*obj);
      cfg.physical = readPhysicalInput(*obj);
    }

    PhysicalObjectFactory factory;
    return factory.create(registry, cfg);
  }
};

class WeaponJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    WeaponConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.transform = readTransformInput(*obj, ctx);
      cfg.transform.name = data::getStringOr(*obj, "name", cfg.transform.name);
      cfg.viewable = readViewableInput(*obj);
      cfg.physical = readPhysicalInput(*obj);
    }

    WeaponFactory factory;
    return factory.create(registry, cfg);
  }
};

class ActorJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    ActorConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.transform = readTransformInput(*obj, ctx);
      cfg.transform.name = data::getStringOr(*obj, "name", cfg.transform.name);
      cfg.viewable = readViewableInput(*obj);
      cfg.physical = readPhysicalInput(*obj);
      cfg.skeleton = readSkeletonInput(*obj);
      cfg.stats = readStatsInput(*obj);
    }

    ActorFactory factory;
    return factory.create(registry, cfg);
  }
};

class TerrainJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    TerrainConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg = readTerrainInput(*obj, ctx);
    }

    TerrainFactory factory;
    return factory.create(registry, cfg);
  }
};

class GrassPatchJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    GrassPatchConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.transform = readTransformInput(*obj, ctx);
      cfg.transform.name = data::getStringOr(*obj, "name", cfg.transform.name);
      cfg.grass = readGrassInput(*obj);
    }

    GrassPatchFactory factory;
    return factory.create(registry, cfg);
  }
};

class VfxJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    VfxConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg = readVfxInput(*obj, ctx);
    }

    VfxFactory factory;
    return factory.create(registry, cfg);
  }
};

class CombatantJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    CombatantConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.transform = readTransformInput(*obj, ctx);
      cfg.transform.name = data::getStringOr(*obj, "name", cfg.transform.name);
      cfg.viewable = readViewableInput(*obj);
      cfg.physical = readPhysicalInput(*obj);
      cfg.skeleton = readSkeletonInput(*obj);
      cfg.stats = readStatsInput(*obj);
    }

    CombatantFactory factory;
    return factory.create(registry, cfg);
  }
};

}  // namespace

void registerFactoriesFromEcsFactoriesDir(EntityFactoryRegistry& out) {
  out.registerFactory("object", std::make_unique<ObjectJsonFactory>());
  out.registerFactory("physical_object", std::make_unique<PhysicalObjectJsonFactory>());
  out.registerFactory("actor", std::make_unique<ActorJsonFactory>());
  out.registerFactory("terrain", std::make_unique<TerrainJsonFactory>());
  out.registerFactory("grass_patch", std::make_unique<GrassPatchJsonFactory>());
  out.registerFactory("vfx", std::make_unique<VfxJsonFactory>());
  out.registerFactory("combatant", std::make_unique<CombatantJsonFactory>());
  out.registerFactory("weapon", std::make_unique<WeaponJsonFactory>());
}

}  // namespace ecs::services
