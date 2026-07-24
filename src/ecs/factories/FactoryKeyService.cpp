#include "ecs/factories/FactoryKeyService.h"

// Author: Karl-Johan Bailey

#include "data/JsonUtil.h"
#include "ecs/EntityRegistry.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/factories/AssetFactory.h"
#include "ecs/factories/BuildingFactory.h"
#include "ecs/factories/CharacterFactory.h"
#include "ecs/factories/EnemyFactory.h"
#include "ecs/factories/EntityFactory.h"
#include "ecs/factories/TerrainFactory.h"
#include "ecs/factories/WeaponFactory.h"
#include "ecs/services/IEntityFactory.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace ecs::services {

namespace {

math::Vec3 readWorldOrLocalPosition(const data::JsonValue::Object& obj, const FactoryContext& ctx) {
  math::Vec3 position{0.0f, 0.0f, 0.0f};
  if (const auto* v = data::getObjectKey(obj, "position")) (void)data::readVec3(*v, position);
  if (const auto* v = data::getObjectKey(obj, "positionLocal")) {
    math::Vec3 local{};
    if (data::readVec3(*v, local)) position = ctx.chunkOriginWorld + local;
  }
  return position;
}

class EntityJsonFactory final : public IEntityFactory {
 public:
  EntityId create(EntityRegistry& registry, const data::JsonValue& config, const FactoryContext& ctx) override {
    EntityConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.name = data::getStringOr(*obj, "name", "");
      cfg.position = readWorldOrLocalPosition(*obj, ctx);
      if (const auto* v = data::getObjectKey(*obj, "rotationDeg")) (void)data::readVec3(*v, cfg.rotationDeg);
      if (const auto* v = data::getObjectKey(*obj, "scale")) (void)data::readVec3(*v, cfg.scale);

      if (const auto* rbCfg = data::getObjectKey(*obj, "rigidbody")) {
        if (const auto* rbObj = rbCfg->tryObject()) {
          cfg.mass = data::getFloatOr(*rbObj, "mass", 0.0f);
          cfg.useGravity = data::getBoolOr(*rbObj, "useGravity", true);
          cfg.hasRigidbody = cfg.mass > 0.0001f;
        }
      }
    }

    EntityFactory f;
    return f.create(registry, cfg);
  }
};

class CharacterJsonFactory final : public IEntityFactory {
 public:
  EntityId create(EntityRegistry& registry, const data::JsonValue& config, const FactoryContext& ctx) override {
    CharacterConfig ccfg{};

    std::string name;
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 rotation{0.0f, 0.0f, 0.0f};
    float massOverride = 0.0f;

    if (const auto* obj = config.tryObject()) {
      if (const auto* v = data::getObjectKey(*obj, "name")) (void)data::readString(*v, name);
      if (const auto* v = data::getObjectKey(*obj, "meshReference")) (void)data::readString(*v, ccfg.meshReference);
      if (const auto* v = data::getObjectKey(*obj, "leftHandKey")) (void)data::readString(*v, ccfg.leftHandKey);
      if (const auto* v = data::getObjectKey(*obj, "rightHandKey")) (void)data::readString(*v, ccfg.rightHandKey);
      if (const auto* v = data::getObjectKey(*obj, "headKey")) (void)data::readString(*v, ccfg.headKey);

      position = readWorldOrLocalPosition(*obj, ctx);
      if (const auto* v = data::getObjectKey(*obj, "rotationDeg")) (void)data::readVec3(*v, rotation);
      if (const auto* v = data::getObjectKey(*obj, "mass")) (void)data::readFloat(*v, massOverride);
    }

    CharacterFactory factory;
    EntityId id = factory.create(registry, ccfg);
    if (id == kInvalidEntityId) return id;

    if (!name.empty()) {
      registry.identity(id).name = name;
    }

    if (auto* tr = registry.tryGet<ecs::TransformComponent>(id)) {
      tr->position = position;
      tr->rotation = rotation;
    }

    // CharacterFactory currently creates a rigidbody with a default mass; allow override.
    if (massOverride > 0.0001f) {
      if (auto* rb = registry.tryGet<ecs::RigidbodyComponent>(id)) {
        rb->mass = massOverride;
        rb->inverseMass = 1.0f / std::max(0.0001f, rb->mass);
      }
    }

    return id;
  }
};

class TerrainJsonFactory final : public IEntityFactory {
 public:
  EntityId create(EntityRegistry& registry, const data::JsonValue& config, const FactoryContext& ctx) override {
    TerrainConfig cfg{};

    if (const auto* obj = config.tryObject()) {
      cfg.name = data::getStringOr(*obj, "name", "terrain");
      cfg.position = readWorldOrLocalPosition(*obj, ctx);
      cfg.gridWidth = data::getIntOr(*obj, "gridWidth", cfg.gridWidth);
      cfg.gridHeight = data::getIntOr(*obj, "gridHeight", cfg.gridHeight);
      cfg.cellSizeMeters = data::getFloatOr(*obj, "cellSizeMeters", cfg.cellSizeMeters);
      cfg.heightScaleMeters = data::getFloatOr(*obj, "heightScaleMeters", cfg.heightScaleMeters);
      cfg.shaderKey = data::getStringOr(*obj, "shaderKey", cfg.shaderKey);
      cfg.depthWrite = data::getBoolOr(*obj, "depthWrite", cfg.depthWrite);
      cfg.colliderEnabled = data::getBoolOr(*obj, "colliderEnabled", cfg.colliderEnabled);
      cfg.colliderThicknessMeters = data::getFloatOr(*obj, "colliderThicknessMeters", cfg.colliderThicknessMeters);
      cfg.lodMaxRenderDistance = data::getFloatOr(*obj, "lodMaxRenderDistance", cfg.lodMaxRenderDistance);
      cfg.lodStep1Distance = data::getFloatOr(*obj, "lodStep1Distance", cfg.lodStep1Distance);
      cfg.lodStep2Distance = data::getFloatOr(*obj, "lodStep2Distance", cfg.lodStep2Distance);
      cfg.lodStep4Distance = data::getFloatOr(*obj, "lodStep4Distance", cfg.lodStep4Distance);
      cfg.lodStep8Distance = data::getFloatOr(*obj, "lodStep8Distance", cfg.lodStep8Distance);
      cfg.lodStep16Distance = data::getFloatOr(*obj, "lodStep16Distance", cfg.lodStep16Distance);
      cfg.lodForceNearDistance = data::getFloatOr(*obj, "lodForceNearDistance", cfg.lodForceNearDistance);
      cfg.tessLockDistance = data::getFloatOr(*obj, "tessLockDistance", cfg.tessLockDistance);
      cfg.tessEnableDistance = data::getFloatOr(*obj, "tessEnableDistance", cfg.tessEnableDistance);
      cfg.tessDisableDistance = data::getFloatOr(*obj, "tessDisableDistance", cfg.tessDisableDistance);
      cfg.viewDotBias = data::getFloatOr(*obj, "viewDotBias", cfg.viewDotBias);

      if (const auto* noiseV = data::getObjectKey(*obj, "noise")) {
        if (const auto* no = noiseV->tryObject()) {
          cfg.noise.seed = static_cast<std::uint32_t>(data::getIntOr(*no, "seed", static_cast<int>(cfg.noise.seed)));
          cfg.noise.frequency = data::getFloatOr(*no, "frequency", cfg.noise.frequency);
          cfg.noise.octaves = data::getIntOr(*no, "octaves", cfg.noise.octaves);
          cfg.noise.lacunarity = data::getFloatOr(*no, "lacunarity", cfg.noise.lacunarity);
          cfg.noise.persistence = data::getFloatOr(*no, "persistence", cfg.noise.persistence);
        }
      }
    }

    TerrainFactory f;
    return f.create(registry, cfg);
  }
};

template <typename FactoryT, typename ConfigT>
class ExistingFactoryWithTransform final : public IEntityFactory {
 public:
  explicit ExistingFactoryWithTransform(FactoryT factory = {}) : m_factory(std::move(factory)) {}

  EntityId create(EntityRegistry& registry, const data::JsonValue& config, const FactoryContext& ctx) override {
    std::string name;
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 rotation{0.0f, 0.0f, 0.0f};

    if (const auto* obj = config.tryObject()) {
      if (const auto* v = data::getObjectKey(*obj, "name")) (void)data::readString(*v, name);
      position = readWorldOrLocalPosition(*obj, ctx);
      if (const auto* v = data::getObjectKey(*obj, "rotationDeg")) (void)data::readVec3(*v, rotation);
    }

    ConfigT cfg{};
    EntityId id = m_factory.create(registry, cfg);
    if (id == kInvalidEntityId) return id;

    if (!name.empty()) {
      registry.identity(id).name = name;
    }

    auto* tr = registry.tryGet<ecs::TransformComponent>(id);
    if (!tr) {
      tr = &registry.emplace<ecs::TransformComponent>(id);
    }
    tr->position = position;
    tr->rotation = rotation;

    return id;
  }

 private:
  FactoryT m_factory;
};

}  // namespace

void registerFactoriesFromEcsFactoriesDir(EntityFactoryRegistry& out) {
  // Keys are intended to match factory concepts in `src/ecs/factories/*Factory.*`.
  out.registerFactory("entity", std::make_unique<EntityJsonFactory>());
  out.registerFactory("character", std::make_unique<CharacterJsonFactory>());
  out.registerFactory("enemy", std::make_unique<ExistingFactoryWithTransform<EnemyFactory, EnemyConfig>>());
  out.registerFactory("weapon", std::make_unique<ExistingFactoryWithTransform<WeaponFactory, WeaponConfig>>());
  out.registerFactory("building", std::make_unique<ExistingFactoryWithTransform<BuildingFactory, BuildingConfig>>());
  out.registerFactory("asset", std::make_unique<ExistingFactoryWithTransform<AssetFactory, AssetConfig>>());
  out.registerFactory("terrain", std::make_unique<TerrainJsonFactory>());
}

}  // namespace ecs::services
