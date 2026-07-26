#include "ecs/factories/FactoryKeyService.h"

// Author: Karl-Johan Bailey

#include "data/JsonUtil.h"
#include "ecs/factories/ObjectFactory.h"
#include "ecs/factories/PhysicalObjectFactory.h"
#include "ecs/factories/ActorFactory.h"
#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/WeaponFactory.h"
#include "ecs/services/IEntityFactory.h"

#include <algorithm>
#include <cctype>
#include <string>
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
    }
  }

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
  out.registerFactory("combatant", std::make_unique<CombatantJsonFactory>());
  out.registerFactory("weapon", std::make_unique<WeaponJsonFactory>());
}

}  // namespace ecs::services
