#include "ecs/factories/FactoryKeyService.h"

// Author: Karl-Johan Bailey

#include "data/JsonUtil.h"
#include "ecs/factories/ObjectFactory.h"
#include "ecs/factories/PhysicalObjectFactory.h"
#include "ecs/factories/ActorFactory.h"
#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/GrassPatchFactory.h"
#include "ecs/factories/PlayableCharacterFactory.h"
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

bool readVec2Value(const data::JsonValue& v, math::Vec2& out) {
  if (const auto* a = v.tryArray()) {
    if (a->size() < 2) return false;
    float x = 0.0f, y = 0.0f;
    if (!data::readFloat((*a)[0], x) || !data::readFloat((*a)[1], y)) return false;
    out = {x, y};
    return true;
  }
  if (const auto* o = v.tryObject()) {
    float x = 0.0f, y = 0.0f;
    if (const auto* xv = data::getObjectKey(*o, "x")) (void)data::readFloat(*xv, x);
    if (const auto* yv = data::getObjectKey(*o, "y")) (void)data::readFloat(*yv, y);
    out = {x, y};
    return true;
  }
  return false;
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

render::RenderMode parseRenderMode(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "masked" || value == "cutout") return render::RenderMode::Masked;
  if (value == "transparent" || value == "translucent" || value == "alpha") return render::RenderMode::Transparent;
  if (value == "additive" || value == "add") return render::RenderMode::Additive;
  return render::RenderMode::Opaque;
}

render::CullMode parseCullMode(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "front") return render::CullMode::Front;
  if (value == "none" || value == "off" || value == "disabled" || value == "double") return render::CullMode::None;
  return render::CullMode::Back;
}

render::DepthTest parseDepthTest(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "disabled" || value == "off" || value == "none") return render::DepthTest::Disabled;
  if (value == "less") return render::DepthTest::Less;
  if (value == "equal") return render::DepthTest::Equal;
  if (value == "greater") return render::DepthTest::Greater;
  if (value == "always") return render::DepthTest::Always;
  return render::DepthTest::LessEqual;
}

render::BlendMode parseBlendMode(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "alpha" || value == "transparent" || value == "translucent") return render::BlendMode::Alpha;
  if (value == "premultiplied" || value == "premultipliedalpha" || value == "premultiplied_alpha") {
    return render::BlendMode::PremultipliedAlpha;
  }
  if (value == "additive" || value == "add") return render::BlendMode::Additive;
  if (value == "multiply" || value == "mul") return render::BlendMode::Multiply;
  return render::BlendMode::Disabled;
}

std::vector<render::TextureBinding> readTextureBindings(const data::JsonValue& v);
std::vector<render::MaterialParameter> readMaterialParameters(const data::JsonValue& v);
std::vector<ShaderBreakpointInput> readShaderBreakpoints(const data::JsonValue& v);
SkeletonInput readSkeletonInput(const data::JsonValue::Object& obj);
StatsInput readStatsInput(const data::JsonValue::Object& obj);
PhysicalInput readPhysicalInput(const data::JsonValue::Object& obj);
AnimationInput readAnimationInput(const data::JsonValue::Object& obj);
PoseInput readPoseInput(const data::JsonValue::Object& obj);
IkInput readIkInput(const data::JsonValue::Object& obj);
SensorConeInput readSensorConeInput(const data::JsonValue::Object& obj);
CombatSetupInput readCombatSetupInput(const data::JsonValue::Object& obj);

std::optional<ecs::ShaderComponent> resolveTerrainMaterialPreset(const std::string& preset) {
  if (preset == "Dirt") return materials::presets::Dirt();
  if (preset == "HighQualityDirt") return materials::presets::HighQualityDirt();
  if (preset == "HighQualityDirtRockLayer") return materials::presets::HighQualityDirtRockLayer();
  if (preset == "HighQualityDirtRockGrassLayer") return materials::presets::HighQualityDirtRockGrassLayer();
  if (preset == "MudFields") return materials::presets::MudFields();
  if (preset == "Mulch") return materials::presets::Mulch();
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
  if (const auto* v = data::getObjectKey(*obj, "renderMode")) {
    std::string s;
    if (data::readString(*v, s)) shader->renderMode = parseRenderMode(std::move(s));
  }
  if (const auto* v = data::getObjectKey(*obj, "cullMode")) {
    std::string s;
    if (data::readString(*v, s)) shader->cullMode = parseCullMode(std::move(s));
  }
  if (const auto* v = data::getObjectKey(*obj, "depthTest")) {
    std::string s;
    if (data::readString(*v, s)) shader->depthTest = parseDepthTest(std::move(s));
  }
  if (const auto* v = data::getObjectKey(*obj, "blendMode")) {
    std::string s;
    if (data::readString(*v, s)) shader->blendMode = parseBlendMode(std::move(s));
  }
  shader->depthWrite = data::getBoolOr(*obj, "depthWrite", shader->depthWrite);
  shader->doubleSided = data::getBoolOr(*obj, "doubleSided", shader->doubleSided);
  shader->castShadows = data::getBoolOr(*obj, "castShadows", shader->castShadows);
  shader->receiveShadows = data::getBoolOr(*obj, "receiveShadows", shader->receiveShadows);

  return shader;
}

void upsertTextureBindingsBySlot(
    std::vector<render::TextureBinding>& dst,
    const std::vector<render::TextureBinding>& overrides) {
  // Texture extraction prefers the first matching slot, so ensure overrides go first.
  for (auto it = overrides.rbegin(); it != overrides.rend(); ++it) {
    const render::TextureBinding& o = *it;
    dst.erase(std::remove_if(dst.begin(), dst.end(), [&](const render::TextureBinding& t) { return t.slot == o.slot; }), dst.end());
    dst.insert(dst.begin(), o);
  }
}

void upsertMaterialParametersByName(
    std::vector<render::MaterialParameter>& dst,
    const std::vector<render::MaterialParameter>& overrides) {
  // Parameter extraction walks the full list, so later parameters win.
  for (const auto& o : overrides) {
    dst.erase(std::remove_if(dst.begin(), dst.end(), [&](const render::MaterialParameter& p) { return p.name == o.name; }), dst.end());
    dst.push_back(o);
  }
}

std::optional<ecs::ShaderComponent> readTerrainMaterialMerged(const data::JsonValue& v) {
  const auto* obj = v.tryObject();
  if (!obj) return std::nullopt;

  ecs::ShaderComponent shader{};
  if (const auto* presetV = data::getObjectKey(*obj, "preset")) {
    std::string preset;
    if (data::readString(*presetV, preset)) {
      if (auto base = resolveTerrainMaterialPreset(preset)) shader = std::move(*base);
    }
  }

  if (const auto* tx = data::getObjectKey(*obj, "textures")) {
    upsertTextureBindingsBySlot(shader.textures, readTextureBindings(*tx));
  }
  if (const auto* pv = data::getObjectKey(*obj, "parameters")) {
    upsertMaterialParametersByName(shader.parameters, readMaterialParameters(*pv));
  }

  return shader;
}

const render::TextureBinding* findEnabledTextureSlot(
    const ecs::ShaderComponent& shader,
    std::initializer_list<const char*> slots) {
  for (const auto& s : slots) {
    for (const auto& t : shader.textures) {
      if (t.slot != s) continue;
      if (!t.texture.enabled || t.texture.key.empty()) continue;
      return &t;
    }
  }
  return nullptr;
}

const render::MaterialParameter* findMaterialParam(
    const ecs::ShaderComponent& shader,
    const char* name) {
  for (const auto& p : shader.parameters) {
    if (p.name == name) return &p;
  }
  return nullptr;
}

void appendSplatMaterialChannelOverrides(
    const ecs::ShaderComponent& material,
    char channel,
    ViewableInput& out) {
  const std::string prefix = std::string("splat_") + channel + "_";

  auto pushTex = [&](const char* suffix, const render::TextureBinding* src) {
    if (!src) return;
    render::TextureBinding t = *src;
    t.slot = prefix + suffix;
    out.textures.push_back(std::move(t));
  };

  // Map a material's base texture set onto the terrain shader's rock layer for the selected splat channel.
  pushTex("rock_albedo", findEnabledTextureSlot(material, {"albedo"}));
  pushTex("rock_normalgl", findEnabledTextureSlot(material, {"normalgl", "normal"}));
  pushTex("rock_roughness", findEnabledTextureSlot(material, {"roughness"}));
  pushTex("rock_ao", findEnabledTextureSlot(material, {"ao", "ambient_occlusion"}));
  pushTex("rock_displacement", findEnabledTextureSlot(material, {"displacement"}));

  // Channel-specific rock-layer parameters.
  out.parameters.push_back({prefix + "rockLayerEnabled", true});

  if (const auto* p = findMaterialParam(material, "uvTiling")) {
    if (const auto* v = std::get_if<math::Vec2>(&p->value)) out.parameters.push_back({prefix + "rockUvTiling", *v});
  }
  if (const auto* p = findMaterialParam(material, "normalScale")) {
    if (const auto* v = std::get_if<float>(&p->value)) out.parameters.push_back({prefix + "rockNormalScale", *v});
  }
  if (const auto* p = findMaterialParam(material, "displacementStrength")) {
    if (const auto* v = std::get_if<float>(&p->value)) out.parameters.push_back({prefix + "rockDisplacementStrength", *v});
  }
  if (const auto* p = findMaterialParam(material, "rockBlendStrength")) {
    if (const auto* v = std::get_if<float>(&p->value)) out.parameters.push_back({prefix + "rockBlendStrength", *v});
  }
  if (const auto* p = findMaterialParam(material, "rockNoiseScale")) {
    if (const auto* v = std::get_if<float>(&p->value)) out.parameters.push_back({prefix + "rockNoiseScale", *v});
  }
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

bool readMaterialValue(const data::JsonValue& v, render::MaterialParamValue& out);

std::string canonicalTextureSlot(std::string key) {
  for (char& ch : key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (key == "basecolor" || key == "base_color" || key == "albedo" || key == "diffuse") return "albedo";
  if (key == "normal" || key == "normalgl" || key == "normal_gl") return "normalgl";
  if (key == "roughness") return "roughness";
  if (key == "metallic") return "metallic";
  if (key == "ao" || key == "ambientocclusion" || key == "ambient_occlusion" || key == "occlusion") return "ao";
  if (key == "emissive") return "emissive";
  if (key == "specular") return "specular";
  if (key == "displacement" || key == "height") return "displacement";
  if (key == "metallicroughness" || key == "metallic_roughness") return "metallicRoughness";
  if (key == "orm") return "orm";
  return key;
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

void appendMaterialTexturesFromObject(const data::JsonValue::Object& obj, std::vector<render::TextureBinding>& out) {
  for (const auto& [key, value] : obj) {
    render::TextureBinding binding{};
    binding.slot = canonicalTextureSlot(key);
    if (const auto* vo = value.tryObject()) {
      binding = readTextureBinding(*vo);
      binding.slot = binding.slot.empty() ? canonicalTextureSlot(key) : canonicalTextureSlot(binding.slot);
    } else {
      std::string textureKey;
      if (!data::readString(value, textureKey)) continue;
      binding.texture.enabled = true;
      binding.texture.key = std::move(textureKey);
      binding.srgb = (binding.slot == "albedo" || binding.slot == "emissive");
    }
    out.push_back(std::move(binding));
  }
}

void appendMaterialParametersFromObject(const data::JsonValue::Object& obj, std::vector<render::MaterialParameter>& out) {
  for (const auto& [key, value] : obj) {
    render::MaterialParameter param{};
    if (key == "sink" || key == "sinkStrength") param.name = "dirtSinkStrength";
    else if (key == "uvScale") param.name = "uvTiling";
    else param.name = key;
    if (!readMaterialValue(value, param.value)) continue;
    out.push_back(std::move(param));
  }
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
  if (const auto* mv = data::getObjectKey(obj, "material")) {
    if (const auto* mo = mv->tryObject()) {
      if (const auto* tx = data::getObjectKey(*mo, "textures")) {
        if (const auto* txObj = tx->tryObject()) appendMaterialTexturesFromObject(*txObj, v.textures);
        else upsertTextureBindingsBySlot(v.textures, readTextureBindings(*tx));
      }
      if (const auto* maps = data::getObjectKey(*mo, "maps")) {
        if (const auto* mapsObj = maps->tryObject()) appendMaterialTexturesFromObject(*mapsObj, v.textures);
      }
      if (const auto* pv = data::getObjectKey(*mo, "parameters")) {
        upsertMaterialParametersByName(v.parameters, readMaterialParameters(*pv));
      }
      if (const auto* values = data::getObjectKey(*mo, "values")) {
        if (const auto* valuesObj = values->tryObject()) appendMaterialParametersFromObject(*valuesObj, v.parameters);
      }
      if (const auto* shading = data::getObjectKey(*mo, "shading")) {
        if (const auto* shadingObj = shading->tryObject()) appendMaterialParametersFromObject(*shadingObj, v.parameters);
      }
      if (const auto* lv = data::getObjectKey(*mo, "lodBreakpoints")) v.lodBreakpoints = readShaderBreakpoints(*lv);
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
  t.lodStep32Distance = data::getFloatOr(obj, "lodStep32Distance", t.lodStep32Distance);
  t.lodForceNearDistance = data::getFloatOr(obj, "lodForceNearDistance", t.lodForceNearDistance);
  t.tessLockDistance = data::getFloatOr(obj, "tessLockDistance", t.tessLockDistance);
  t.tessEnableDistance = data::getFloatOr(obj, "tessEnableDistance", t.tessEnableDistance);
  t.tessDisableDistance = data::getFloatOr(obj, "tessDisableDistance", t.tessDisableDistance);
  t.viewDotBias = data::getFloatOr(obj, "viewDotBias", t.viewDotBias);
  if (const auto* farLodV = data::getObjectKey(obj, "farLod")) {
    if (const auto* farLod = farLodV->tryObject()) {
      t.farLod.enabled = data::getBoolOr(*farLod, "enabled", t.farLod.enabled);
      t.farLod.startDistance = data::getFloatOr(*farLod, "startDistance", t.farLod.startDistance);
      t.farLod.endDistance = data::getFloatOr(*farLod, "endDistance", t.farLod.endDistance);
      t.farLod.billboardScale = data::getFloatOr(*farLod, "billboardScale", t.farLod.billboardScale);
      t.farLod.heightOffset = data::getFloatOr(*farLod, "heightOffset", t.farLod.heightOffset);
      t.farLod.cameraFacing = data::getBoolOr(*farLod, "cameraFacing", t.farLod.cameraFacing);
      if (const auto* texture = data::getObjectKey(*farLod, "texture")) {
        std::string key;
        if (data::readString(*texture, key) && !key.empty()) {
          t.farLod.texture.enabled = true;
          t.farLod.texture.key = std::move(key);
        }
      }
      if (const auto* tint = data::getObjectKey(*farLod, "tint")) {
        if (const auto* arr = tint->tryArray()) {
          float r = t.farLod.tint.r;
          float g = t.farLod.tint.g;
          float b = t.farLod.tint.b;
          float a = t.farLod.tint.a;
          if (arr->size() >= 3 && data::readFloat((*arr)[0], r) && data::readFloat((*arr)[1], g) &&
              data::readFloat((*arr)[2], b)) {
            if (arr->size() >= 4) (void)data::readFloat((*arr)[3], a);
            t.farLod.tint = {r, g, b, a};
          }
        }
      }
    }
  }
  t.farLod.startDistance = std::max(t.lodMaxRenderDistance, t.farLod.startDistance);
  t.farLod.endDistance = std::max(t.farLod.startDistance + 1.0f, t.farLod.endDistance);
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
  if (const auto* smv = data::getObjectKey(obj, "splatMaterials")) {
    if (const auto* smo = smv->tryObject()) {
      auto applyChannel = [&](const char* key, char channel) {
        if (const auto* cv = data::getObjectKey(*smo, key)) {
          if (auto mat = readTerrainMaterialMerged(*cv)) {
            appendSplatMaterialChannelOverrides(*mat, channel, t.viewable);
          }
        }
      };

      applyChannel("r", 'r');
      applyChannel("g", 'g');
      applyChannel("b", 'b');
      applyChannel("a", 'a');
      // Allow uppercase keys too.
      applyChannel("R", 'r');
      applyChannel("G", 'g');
      applyChannel("B", 'b');
      applyChannel("A", 'a');
    }
  }
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

void readCombatantHudInto(const data::JsonValue::Object& obj, CombatantHudInput& hud) {
  const auto* hv = data::getObjectKey(obj, "combatantHud");
  if (!hv) hv = data::getObjectKey(obj, "hud");
  const auto* ho = hv ? hv->tryObject() : nullptr;
  if (!ho) return;

  hud.enabled = data::getBoolOr(*ho, "enabled", hud.enabled);
  hud.texturePath = data::getStringOr(*ho, "texturePath", hud.texturePath);
  if (const auto* v = data::getObjectKey(*ho, "worldOffset")) (void)data::readVec3(*v, hud.worldOffset);
  hud.heightMeters = data::getFloatOr(*ho, "heightMeters", hud.heightMeters);
  hud.maxRenderDistanceMeters = data::getFloatOr(*ho, "maxRenderDistanceMeters", hud.maxRenderDistanceMeters);
  if (const auto* v = data::getObjectKey(*ho, "maxRenderDistance")) (void)data::readFloat(*v, hud.maxRenderDistanceMeters);
  hud.distanceScaleEnabled = data::getBoolOr(*ho, "distanceScaleEnabled", hud.distanceScaleEnabled);
  hud.distanceScaleStartMeters = data::getFloatOr(*ho, "distanceScaleStartMeters", hud.distanceScaleStartMeters);
  hud.distanceScaleEndMeters = data::getFloatOr(*ho, "distanceScaleEndMeters", hud.distanceScaleEndMeters);
  hud.distanceScaleAtEnd = data::getFloatOr(*ho, "distanceScaleAtEnd", hud.distanceScaleAtEnd);
  hud.fillEnabled = data::getBoolOr(*ho, "fillEnabled", hud.fillEnabled);
  hud.fillWidthRatio = data::getFloatOr(*ho, "fillWidthRatio", hud.fillWidthRatio);
  hud.fillHeightRatio = data::getFloatOr(*ho, "fillHeightRatio", hud.fillHeightRatio);
  hud.fillDepthBiasMeters = data::getFloatOr(*ho, "fillDepthBiasMeters", hud.fillDepthBiasMeters);
  if (const auto* v = data::getObjectKey(*ho, "fillColor")) (void)readColorValue(*v, hud.fillColor);
  if (const auto* v = data::getObjectKey(*ho, "tint")) (void)readColorValue(*v, hud.tint);
}

void readPlayerHudInto(const data::JsonValue::Object& obj, const char* key, bool allowHudAlias, PlayerHudInput& hud) {
  const auto* hv = data::getObjectKey(obj, key);
  if (!hv && allowHudAlias) hv = data::getObjectKey(obj, "hud");
  const auto* ho = hv ? hv->tryObject() : nullptr;
  if (!ho) return;

  hud.enabled = data::getBoolOr(*ho, "enabled", hud.enabled);
  hud.texturePath = data::getStringOr(*ho, "texturePath", hud.texturePath);
  hud.heightPx = data::getFloatOr(*ho, "heightPx", hud.heightPx);
  hud.marginLeftPx = data::getFloatOr(*ho, "marginLeftPx", hud.marginLeftPx);
  hud.marginBottomPx = data::getFloatOr(*ho, "marginBottomPx", hud.marginBottomPx);
  hud.flipU = data::getBoolOr(*ho, "flipU", hud.flipU);
  hud.flipV = data::getBoolOr(*ho, "flipV", hud.flipV);
  hud.fillEnabled = data::getBoolOr(*ho, "fillEnabled", hud.fillEnabled);
  hud.fillLayer = data::getIntOr(*ho, "fillLayer", hud.fillLayer);
  hud.fillWidthRatio = data::getFloatOr(*ho, "fillWidthRatio", hud.fillWidthRatio);
  hud.fillHeightRatio = data::getFloatOr(*ho, "fillHeightRatio", hud.fillHeightRatio);
  hud.fillFromRight = data::getBoolOr(*ho, "fillFromRight", hud.fillFromRight);
  if (const auto* v = data::getObjectKey(*ho, "fillOffsetPx")) (void)readVec2Value(*v, hud.fillOffsetPx);
  if (const auto* v = data::getObjectKey(*ho, "fillColor")) (void)readColorValue(*v, hud.fillColor);
  if (const auto* v = data::getObjectKey(*ho, "tint")) (void)readColorValue(*v, hud.tint);
}

PlayableCharacterConfig readPlayableCharacterInput(const data::JsonValue::Object& obj, const FactoryContext& ctx) {
  PlayableCharacterConfig pc{};

  pc.base.transform = readTransformInput(obj, ctx);
  pc.base.transform.name = data::getStringOr(obj, "name", pc.base.transform.name);
  pc.base.viewable = readViewableInput(obj);
  pc.base.physical = readPhysicalInput(obj);
  pc.base.skeleton = readSkeletonInput(obj);
  pc.base.stats = readStatsInput(obj);
  pc.base.animation = readAnimationInput(obj);
  pc.base.pose = readPoseInput(obj);
  pc.base.ik = readIkInput(obj);
  pc.base.sensorCone = readSensorConeInput(obj);
  pc.base.combat = readCombatSetupInput(obj);
  readCombatantHudInto(obj, pc.base.hud);

  pc.hasController = data::getBoolOr(obj, "hasController", pc.hasController);
  readPlayerHudInto(obj, "playerHud", true, pc.hud);

  if (const auto* cv = data::getObjectKey(obj, "camera")) {
    if (const auto* co = cv->tryObject()) {
      pc.camera.transform = readTransformInput(*co, ctx);
      pc.camera.transform.name = data::getStringOr(*co, "name", pc.camera.transform.name);
      if (const auto* v = data::getObjectKey(*co, "targetOffset")) (void)data::readVec3(*v, pc.camera.targetOffset);
      pc.camera.distance = data::getFloatOr(*co, "distance", pc.camera.distance);
      pc.camera.height = data::getFloatOr(*co, "height", pc.camera.height);
      pc.camera.pitchDeg = data::getFloatOr(*co, "pitchDeg", pc.camera.pitchDeg);
      pc.camera.minPitchDeg = data::getFloatOr(*co, "minPitchDeg", pc.camera.minPitchDeg);
      pc.camera.maxPitchDeg = data::getFloatOr(*co, "maxPitchDeg", pc.camera.maxPitchDeg);

      pc.camera.depthOfFieldEnabled = data::getBoolOr(*co, "depthOfFieldEnabled", pc.camera.depthOfFieldEnabled);
      pc.camera.dofFocusRange = data::getFloatOr(*co, "dofFocusRange", pc.camera.dofFocusRange);
      pc.camera.dofBlurStrength = data::getFloatOr(*co, "dofBlurStrength", pc.camera.dofBlurStrength);
      if (const auto* v = data::getObjectKey(*co, "dofFocusTargetOffset")) (void)data::readVec3(*v, pc.camera.dofFocusTargetOffset);

      pc.camera.motionBlurEnabled = data::getBoolOr(*co, "motionBlurEnabled", pc.camera.motionBlurEnabled);
      pc.camera.motionBlurStrength = data::getFloatOr(*co, "motionBlurStrength", pc.camera.motionBlurStrength);
      pc.camera.motionBlurMaxBlurPixels = data::getFloatOr(*co, "motionBlurMaxBlurPixels", pc.camera.motionBlurMaxBlurPixels);
      pc.camera.motionBlurSamples = data::getIntOr(*co, "motionBlurSamples", pc.camera.motionBlurSamples);
    }
  }

  // Optional nested base config.
  if (const auto* bv = data::getObjectKey(obj, "base")) {
    if (const auto* bo = bv->tryObject()) {
      pc.base.transform = readTransformInput(*bo, ctx);
      pc.base.transform.name = data::getStringOr(*bo, "name", pc.base.transform.name);
      pc.base.viewable = readViewableInput(*bo);
      pc.base.physical = readPhysicalInput(*bo);
      pc.base.skeleton = readSkeletonInput(*bo);
      pc.base.stats = readStatsInput(*bo);
      pc.base.animation = readAnimationInput(*bo);
      pc.base.pose = readPoseInput(*bo);
      pc.base.ik = readIkInput(*bo);
      pc.base.sensorCone = readSensorConeInput(*bo);
      pc.base.combat = readCombatSetupInput(*bo);
      readCombatantHudInto(*bo, pc.base.hud);
    }
  }

  return pc;
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
    v.vfx.turbulence = data::getFloatOr(src, "turbulence", v.vfx.turbulence);
    v.vfx.swirlStrength = data::getFloatOr(src, "swirlStrength", v.vfx.swirlStrength);
    v.vfx.coreSizeMeters = data::getFloatOr(src, "coreSizeMeters", v.vfx.coreSizeMeters);
    v.vfx.glowStrength = data::getFloatOr(src, "glowStrength", v.vfx.glowStrength);
    v.vfx.emberRate = data::getFloatOr(src, "emberRate", v.vfx.emberRate);
    v.vfx.smokeAmount = data::getFloatOr(src, "smokeAmount", v.vfx.smokeAmount);
    v.vfx.chargeLengthMeters = data::getFloatOr(src, "chargeLengthMeters", v.vfx.chargeLengthMeters);
    v.vfx.arcJitter = data::getFloatOr(src, "arcJitter", v.vfx.arcJitter);
    v.vfx.branchCount = data::getIntOr(src, "branchCount", v.vfx.branchCount);
    v.vfx.segmentCount = data::getIntOr(src, "segmentCount", v.vfx.segmentCount);
    v.vfx.pulseSpeed = data::getFloatOr(src, "pulseSpeed", v.vfx.pulseSpeed);
    v.vfx.arcThickness = data::getFloatOr(src, "arcThickness", v.vfx.arcThickness);
    v.vfx.arcGlow = data::getFloatOr(src, "arcGlow", v.vfx.arcGlow);
    v.vfx.sparkCount = data::getIntOr(src, "sparkCount", v.vfx.sparkCount);
    v.vfx.sparkSpreadDegrees = data::getFloatOr(src, "sparkSpreadDegrees", v.vfx.sparkSpreadDegrees);
    v.vfx.sparkTrailLengthMeters = data::getFloatOr(src, "sparkTrailLengthMeters", v.vfx.sparkTrailLengthMeters);
    v.vfx.sparkFadeSeconds = data::getFloatOr(src, "sparkFadeSeconds", v.vfx.sparkFadeSeconds);
    v.vfx.sparkBurstJitter = data::getFloatOr(src, "sparkBurstJitter", v.vfx.sparkBurstJitter);
    v.vfx.sparkGravityScale = data::getFloatOr(src, "sparkGravityScale", v.vfx.sparkGravityScale);
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
      p.colliderMeshId = data::getStringOr(*po, "meshId", p.colliderMeshId);
      p.colliderMeshKey = data::getStringOr(*po, "meshKey", p.colliderMeshKey);
      p.colliderUseMeshBounds = data::getBoolOr(*po, "useMeshBounds", p.colliderUseMeshBounds);
    }
  }
  return p;
}

AnimationInput readAnimationInput(const data::JsonValue::Object& obj) {
  AnimationInput a{};

  const auto* av = data::getObjectKey(obj, "animation");
  const auto* ao = av ? av->tryObject() : nullptr;
  if (!ao) return a;

  a.enabled = data::getBoolOr(*ao, "enabled", a.enabled);
  a.idleDelaySeconds = data::getFloatOr(*ao, "idleDelaySeconds", a.idleDelaySeconds);
  a.idleAnimationClip = data::getStringOr(*ao, "idleAnimationClip", a.idleAnimationClip);
  a.idleAnimationKey = data::getStringOr(*ao, "idleAnimationKey", a.idleAnimationKey);
  a.idleAnimationLayer = data::getStringOr(*ao, "idleAnimationLayer", a.idleAnimationLayer);
  a.locomotionIdleKey = data::getStringOr(*ao, "locomotionIdleKey", a.locomotionIdleKey);
  a.locomotionWalkKey = data::getStringOr(*ao, "locomotionWalkKey", a.locomotionWalkKey);
  a.locomotionRunKey = data::getStringOr(*ao, "locomotionRunKey", a.locomotionRunKey);

  if (const auto* clipsV = data::getObjectKey(*ao, "availableClips")) {
    a.availableClips = readStringArrayOrEmpty(*clipsV);
  }
  if (const auto* bindingV = data::getObjectKey(*ao, "clips")) {
    if (const auto* arr = bindingV->tryArray()) {
      a.clipBindings.clear();
      a.clipBindings.reserve(arr->size());
      for (const auto& el : *arr) {
        if (const auto* bo = el.tryObject()) {
          AnimationClipBindingInput binding{};
          binding.key = data::getStringOr(*bo, "key", binding.key);
          binding.clip = data::getStringOr(*bo, "clip", binding.clip);
          binding.speed = data::getFloatOr(*bo, "speed", binding.speed);
          if (!binding.key.empty() && !binding.clip.empty()) a.clipBindings.push_back(std::move(binding));
        }
      }
    }
  }
  if (const auto* locomotionV = data::getObjectKey(*ao, "locomotion")) {
    if (const auto* locomotionO = locomotionV->tryObject()) {
      a.locomotionIdleKey = data::getStringOr(*locomotionO, "idle", a.locomotionIdleKey);
      a.locomotionWalkKey = data::getStringOr(*locomotionO, "walk", a.locomotionWalkKey);
      a.locomotionRunKey = data::getStringOr(*locomotionO, "run", a.locomotionRunKey);
    }
  }

  if (const auto* layersV = data::getObjectKey(*ao, "layers")) {
    if (const auto* arr = layersV->tryArray()) {
      a.layers.clear();
      a.layers.reserve(arr->size());
      for (const auto& el : *arr) {
        if (const auto* lo = el.tryObject()) {
          AnimationLayerInput l{};
          l.name = data::getStringOr(*lo, "name", l.name);
          l.weight = data::getFloatOr(*lo, "weight", l.weight);
          l.blendMode = data::getStringOr(*lo, "blendMode", l.blendMode);
          if (const auto* mv = data::getObjectKey(*lo, "mask")) l.mask = readStringArrayOrEmpty(*mv);
          l.currentState = data::getStringOr(*lo, "currentStateKey", l.currentState);
          l.currentState = data::getStringOr(*lo, "currentState", l.currentState);
          l.nextState = data::getStringOr(*lo, "nextStateKey", l.nextState);
          l.nextState = data::getStringOr(*lo, "nextState", l.nextState);
          l.transition = data::getFloatOr(*lo, "transition", l.transition);
          a.layers.push_back(std::move(l));
        }
      }
    }
  }

  return a;
}

PoseInput readPoseInput(const data::JsonValue::Object& obj) {
  PoseInput p{};

  const auto* pv = data::getObjectKey(obj, "pose");
  const auto* po = pv ? pv->tryObject() : nullptr;
  if (!po) return p;

  p.enabled = data::getBoolOr(*po, "enabled", p.enabled);
  p.defaultPoseName = data::getStringOr(*po, "defaultPoseName", p.defaultPoseName);
  p.defaultPoseEnabled = data::getBoolOr(*po, "defaultPoseEnabled", p.defaultPoseEnabled);
  p.defaultPoseWeight = data::getFloatOr(*po, "defaultPoseWeight", p.defaultPoseWeight);
  if (const auto* posesV = data::getObjectKey(*po, "poses")) {
    if (const auto* arr = posesV->tryArray()) {
      p.poses.clear();
      p.poses.reserve(arr->size());
      for (const auto& poseV : *arr) {
        const auto* poseO = poseV.tryObject();
        if (!poseO) continue;
        PoseDefinitionInput pose{};
        pose.name = data::getStringOr(*poseO, "name", pose.name);
        pose.enabled = data::getBoolOr(*poseO, "enabled", pose.enabled);
        pose.weight = data::getFloatOr(*poseO, "weight", pose.weight);
        if (const auto* bonesV = data::getObjectKey(*poseO, "bones")) {
          if (const auto* bonesA = bonesV->tryArray()) {
            pose.bones.reserve(bonesA->size());
            for (const auto& boneV : *bonesA) {
              const auto* boneO = boneV.tryObject();
              if (!boneO) continue;
              PoseBoneOverrideInput bone{};
              bone.boneKey = data::getStringOr(*boneO, "boneKey", bone.boneKey);
              bone.boneKey = data::getStringOr(*boneO, "bone", bone.boneKey);
              bone.weight = data::getFloatOr(*boneO, "weight", bone.weight);
              if (const auto* t = data::getObjectKey(*boneO, "translation")) {
                bone.hasTranslation = data::readVec3(*t, bone.translation);
              }
              if (const auto* r = data::getObjectKey(*boneO, "rotationEulerDeg")) {
                bone.hasRotationEulerDeg = data::readVec3(*r, bone.rotationEulerDeg);
              }
              if (const auto* r = data::getObjectKey(*boneO, "rotationDeg")) {
                bone.hasRotationEulerDeg = data::readVec3(*r, bone.rotationEulerDeg);
              }
              if (const auto* s = data::getObjectKey(*boneO, "scale")) {
                bone.hasScale = data::readVec3(*s, bone.scale);
              }
              pose.bones.push_back(std::move(bone));
            }
          }
        }
        p.poses.push_back(std::move(pose));
      }
    }
  }
  return p;
}

static void readIkChainInput(const data::JsonValue& v, IkChainInput& out) {
  const auto* o = v.tryObject();
  if (!o) return;

  out.enabled = data::getBoolOr(*o, "enabled", out.enabled);
  out.name = data::getStringOr(*o, "name", out.name);
  if (const auto* bv = data::getObjectKey(*o, "bones")) out.bones = readStringArrayOrEmpty(*bv);
  if (const auto* ev = data::getObjectKey(*o, "targetEntity")) {
    int n = 0;
    if (data::readInt(*ev, n) && n > 0) out.targetEntity = static_cast<ecs::EntityId>(n);
  }
  if (const auto* ov = data::getObjectKey(*o, "targetOffset")) (void)data::readVec3(*ov, out.targetOffset);
  if (const auto* lv = data::getObjectKey(*o, "targetLocalOffset")) (void)data::readVec3(*lv, out.targetLocalOffset);
  out.weight = data::getFloatOr(*o, "weight", out.weight);
  out.blendInSeconds = data::getFloatOr(*o, "blendInSeconds", out.blendInSeconds);
  out.blendOutSeconds = data::getFloatOr(*o, "blendOutSeconds", out.blendOutSeconds);
  out.iterations = data::getIntOr(*o, "iterations", out.iterations);
  out.overrideAnimation = data::getBoolOr(*o, "overrideAnimation", out.overrideAnimation);
}

IkInput readIkInput(const data::JsonValue::Object& obj) {
  IkInput ik{};

  const auto* iv = data::getObjectKey(obj, "ik");
  const auto* io = iv ? iv->tryObject() : nullptr;
  if (!io) return ik;

  ik.enabled = data::getBoolOr(*io, "enabled", ik.enabled);
  if (const auto* hv = data::getObjectKey(*io, "headLook")) readIkChainInput(*hv, ik.headLook);
  if (const auto* rv = data::getObjectKey(*io, "reachTarget")) readIkChainInput(*rv, ik.reachTarget);
  return ik;
}

SensorConeInput readSensorConeInput(const data::JsonValue::Object& obj) {
  SensorConeInput s{};

  const auto* sv = data::getObjectKey(obj, "sensorCone");
  const auto* so = sv ? sv->tryObject() : nullptr;
  if (!so) return s;

  s.enabled = data::getBoolOr(*so, "enabled", s.enabled);
  s.sensorName = data::getStringOr(*so, "sensorName", s.sensorName);
  s.socketName = data::getStringOr(*so, "socketName", s.socketName);
  if (const auto* v = data::getObjectKey(*so, "socketPositionOffset")) (void)data::readVec3(*v, s.socketPositionOffset);

  if (const auto* cv = data::getObjectKey(*so, "cone")) {
    if (const auto* co = cv->tryObject()) {
      s.cone.rayCount = data::getIntOr(*co, "rayCount", s.cone.rayCount);
      s.cone.coneAngleDeg = data::getFloatOr(*co, "coneAngleDeg", s.cone.coneAngleDeg);
      s.cone.length = data::getFloatOr(*co, "length", s.cone.length);
      s.cone.radius = data::getFloatOr(*co, "radius", s.cone.radius);
      if (const auto* lv = data::getObjectKey(*co, "collisionLayers")) {
        if (const auto* n = lv->tryNumber()) {
          s.cone.collisionLayers = static_cast<physics::LayerMask>(static_cast<std::uint32_t>(*n));
        } else {
          std::string str;
          if (data::readString(*lv, str)) s.cone.collisionLayers = parseLayerMask(std::move(str));
        }
      }
      if (const auto* lv = data::getObjectKey(*co, "ignoreLayers")) {
        if (const auto* n = lv->tryNumber()) {
          s.cone.ignoreLayers = static_cast<physics::LayerMask>(static_cast<std::uint32_t>(*n));
        } else {
          std::string str;
          if (data::readString(*lv, str)) s.cone.ignoreLayers = parseLayerMask(std::move(str));
        }
      }
      s.cone.ignoreSelf = data::getBoolOr(*co, "ignoreSelf", s.cone.ignoreSelf);
      s.cone.maxHits = data::getIntOr(*co, "maxHits", s.cone.maxHits);
      if (const auto* v = data::getObjectKey(*co, "originLocalOffset")) (void)data::readVec3(*v, s.cone.originLocalOffset);
      s.cone.baseName = data::getStringOr(*co, "baseName", s.cone.baseName);
    }
  }

  return s;
}

AttachmentMountInput readAttachmentMountInput(const data::JsonValue::Object& obj) {
  AttachmentMountInput mount{};
  mount.mode = data::getStringOr(obj, "mode", mount.mode);
  mount.targetEntityName = data::getStringOr(obj, "targetEntityName", mount.targetEntityName);
  mount.targetMeshId = data::getStringOr(obj, "targetMeshId", mount.targetMeshId);
  mount.skeletonId = data::getStringOr(obj, "skeletonId", mount.skeletonId);
  mount.boneName = data::getStringOr(obj, "boneName", mount.boneName);
  mount.socketName = data::getStringOr(obj, "socketName", mount.socketName);
  mount.inheritPosition = data::getBoolOr(obj, "inheritPosition", mount.inheritPosition);
  mount.inheritRotation = data::getBoolOr(obj, "inheritRotation", mount.inheritRotation);
  mount.inheritScale = data::getBoolOr(obj, "inheritScale", mount.inheritScale);
  if (const auto* v = data::getObjectKey(obj, "positionOffset")) (void)data::readVec3(*v, mount.positionOffset);
  if (const auto* v = data::getObjectKey(obj, "rotationOffset")) (void)data::readVec3(*v, mount.rotationOffset);
  if (const auto* v = data::getObjectKey(obj, "scaleOffset")) (void)data::readVec3(*v, mount.scaleOffset);
  return mount;
}

ecs::CombatVolumeComponent::Role parseCombatRole(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return value == "hit" ? ecs::CombatVolumeComponent::Role::Hit : ecs::CombatVolumeComponent::Role::Hurt;
}

ecs::CombatVolumeComponent::Shape parseCombatShape(std::string value) {
  for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (value == "sphere") return ecs::CombatVolumeComponent::Shape::Sphere;
  if (value == "capsule") return ecs::CombatVolumeComponent::Shape::Capsule;
  return ecs::CombatVolumeComponent::Shape::Box;
}

CombatSetupInput readCombatSetupInput(const data::JsonValue::Object& obj) {
  CombatSetupInput out{};
  const auto* combatV = data::getObjectKey(obj, "combat");
  const auto* combatO = combatV ? combatV->tryObject() : nullptr;
  if (!combatO) return out;

  if (const auto* volumesV = data::getObjectKey(*combatO, "volumes")) {
    if (const auto* arr = volumesV->tryArray()) {
      out.volumes.reserve(arr->size());
      for (const auto& el : *arr) {
        const auto* vo = el.tryObject();
        if (!vo) continue;
        CombatAttachmentInput attach{};
        attach.name = data::getStringOr(*vo, "name", attach.name);
        attach.sourceMeshId = data::getStringOr(*vo, "sourceMeshId", attach.sourceMeshId);
        if (const auto* mountV = data::getObjectKey(*vo, "attach")) {
          if (const auto* mountO = mountV->tryObject()) attach.mount = readAttachmentMountInput(*mountO);
        }
        if (const auto* listV = data::getObjectKey(*vo, "items")) {
          if (const auto* itemsA = listV->tryArray()) {
            attach.volumes.reserve(itemsA->size());
            for (const auto& itemV : *itemsA) {
              const auto* itemO = itemV.tryObject();
              if (!itemO) continue;
              ecs::CombatVolumeComponent::Volume volume{};
              std::string role = data::getStringOr(*itemO, "role", "hurt");
              std::string shape = data::getStringOr(*itemO, "shape", "box");
              volume.role = parseCombatRole(std::move(role));
              volume.shape = parseCombatShape(std::move(shape));
              if (const auto* ov = data::getObjectKey(*itemO, "offset")) (void)data::readVec3(*ov, volume.offset);
              if (const auto* size = data::getObjectKey(*itemO, "size")) {
                math::Vec3 dims{};
                if (data::readVec3(*size, dims)) {
                  volume.box = {dims.x, dims.y, dims.z};
                  volume.capsule = {dims.x, dims.y};
                  volume.sphere = {dims.x};
                }
              }
              volume.damageMultiplier = data::getFloatOr(*itemO, "damageMultiplier", volume.damageMultiplier);
              volume.damage = data::getFloatOr(*itemO, "damage", volume.damage);
              volume.damageType = data::getStringOr(*itemO, "damageType", volume.damageType);
              volume.force = data::getFloatOr(*itemO, "force", volume.force);
              volume.enabled = data::getBoolOr(*itemO, "enabled", volume.enabled);
              volume.singleHit = data::getBoolOr(*itemO, "singleHit", volume.singleHit);
              if (const auto* tags = data::getObjectKey(*itemO, "tags")) volume.tags = readStringArrayOrEmpty(*tags);
              attach.volumes.push_back(std::move(volume));
            }
          }
        }
        out.volumes.push_back(std::move(attach));
      }
    }
  }

  if (const auto* raysV = data::getObjectKey(*combatO, "raycasts")) {
    if (const auto* arr = raysV->tryArray()) {
      out.raycasts.reserve(arr->size());
      for (const auto& el : *arr) {
        const auto* ro = el.tryObject();
        if (!ro) continue;
        RaycastAttachmentInput ray{};
        ray.name = data::getStringOr(*ro, "name", ray.name);
        ray.sensorName = data::getStringOr(*ro, "sensorName", ray.sensorName);
        ray.createSensor = data::getBoolOr(*ro, "createSensor", ray.createSensor);
        if (const auto* mountV = data::getObjectKey(*ro, "attach")) {
          if (const auto* mountO = mountV->tryObject()) ray.mount = readAttachmentMountInput(*mountO);
        }
        ray.raycast.enabled = data::getBoolOr(*ro, "enabled", ray.raycast.enabled);
        ray.raycast.raycastCategory = data::getStringOr(*ro, "category", ray.raycast.raycastCategory);
        ray.raycast.length = data::getFloatOr(*ro, "length", ray.raycast.length);
        ray.raycast.radius = data::getFloatOr(*ro, "radius", ray.raycast.radius);
        ray.raycast.maxHits = data::getIntOr(*ro, "maxHits", ray.raycast.maxHits);
        ray.raycast.ignoreSelf = data::getBoolOr(*ro, "ignoreSelf", ray.raycast.ignoreSelf);
        if (const auto* v = data::getObjectKey(*ro, "localOffset")) (void)data::readVec3(*v, ray.raycast.localOffset);
        if (const auto* v = data::getObjectKey(*ro, "customDirection")) {
          if (data::readVec3(*v, ray.raycast.customDirection)) ray.raycast.directionMode = ecs::RaycastComponent::DirectionMode::CustomVector;
        }
        out.raycasts.push_back(std::move(ray));
      }
    }
  }

  if (const auto* vfxV = data::getObjectKey(*combatO, "vfx")) {
    if (const auto* arr = vfxV->tryArray()) {
      out.vfx.reserve(arr->size());
      for (const auto& el : *arr) {
        const auto* vo = el.tryObject();
        if (!vo) continue;
        VfxAttachmentInput vfx{};
        vfx.name = data::getStringOr(*vo, "name", vfx.name);
        if (const auto* mountV = data::getObjectKey(*vo, "attach")) {
          if (const auto* mountO = mountV->tryObject()) vfx.mount = readAttachmentMountInput(*mountO);
        }
        std::string type = data::getStringOr(*vo, "type", "fire");
        std::string quality = data::getStringOr(*vo, "quality", "high");
        vfx.vfx.type = parseVfxType(std::move(type));
        vfx.vfx.quality = parseVfxQuality(std::move(quality));
        vfx.vfx.enabled = data::getBoolOr(*vo, "enabled", vfx.vfx.enabled);
        vfx.vfx.intensity = data::getFloatOr(*vo, "intensity", vfx.vfx.intensity);
        vfx.vfx.spawnRate = data::getFloatOr(*vo, "spawnRate", vfx.vfx.spawnRate);
        vfx.vfx.sizeMeters = data::getFloatOr(*vo, "sizeMeters", vfx.vfx.sizeMeters);
        vfx.vfx.glowStrength = data::getFloatOr(*vo, "glowStrength", vfx.vfx.glowStrength);
        vfx.vfx.emberRate = data::getFloatOr(*vo, "emberRate", vfx.vfx.emberRate);
        vfx.vfx.smokeAmount = data::getFloatOr(*vo, "smokeAmount", vfx.vfx.smokeAmount);
        out.vfx.push_back(std::move(vfx));
      }
    }
  }

  return out;
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
      cfg.animation = readAnimationInput(*obj);
      cfg.pose = readPoseInput(*obj);
      cfg.ik = readIkInput(*obj);
      cfg.sensorCone = readSensorConeInput(*obj);
      cfg.combat = readCombatSetupInput(*obj);
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

class PlayableCharacterJsonFactory final : public IEntityFactory {
 public:
  EntityId create(
      EntityRegistry& registry,
      const data::JsonValue& config,
      const FactoryContext& ctx) override {
    PlayableCharacterConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg = readPlayableCharacterInput(*obj, ctx);
    }

    PlayableCharacterFactory factory;
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
      cfg.animation = readAnimationInput(*obj);
      cfg.pose = readPoseInput(*obj);
      cfg.ik = readIkInput(*obj);
      cfg.sensorCone = readSensorConeInput(*obj);
      cfg.combat = readCombatSetupInput(*obj);
      readCombatantHudInto(*obj, cfg.hud);
      readPlayerHudInto(*obj, "playerHud", false, cfg.playerHud);
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
  out.registerFactory("playable_character", std::make_unique<PlayableCharacterJsonFactory>());
  out.registerFactory("vfx", std::make_unique<VfxJsonFactory>());
  out.registerFactory("combatant", std::make_unique<CombatantJsonFactory>());
  out.registerFactory("weapon", std::make_unique<WeaponJsonFactory>());
}

}  // namespace ecs::services
