// Author: Karl-Johan Bailey

#include "ecs/factories/CombatantFactory.h"
#include "ecs/factories/ActorFactory.h"

#include "ecs/components/CharacterComponent.h"
#include "ecs/components/CombatantHudComponent.h"
#include "ecs/components/PlayerHudComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/RigidbodyComponent.h"

namespace ecs::services {

EntityId CombatantFactory::create(
    EntityRegistry& registry,
    const CombatantConfig& config) {
  ActorFactory baseFactory;
  ActorConfig baseCfg{};
  baseCfg.transform = config.transform;
  baseCfg.viewable = config.viewable;
  baseCfg.physical = config.physical;
  baseCfg.skeleton = config.skeleton;
  baseCfg.stats = config.stats;
  baseCfg.animation = config.animation;
  baseCfg.pose = config.pose;
  baseCfg.ik = config.ik;
  baseCfg.sensorCone = config.sensorCone;
  baseCfg.combat = config.combat;
  const EntityId id = baseFactory.create(registry, baseCfg);
  if (id == kInvalidEntityId) return id;

  registry.emplace<ecs::CharacterComponent>(id);

  if (config.hud.enabled) {
    const std::string hudName = config.transform.name.empty() ? "combatant_hud" : (config.transform.name + "_hud");
    const EntityId hudEntity = registry.createEntity(hudName);
    if (hudEntity != kInvalidEntityId) {
      registry.emplace<ecs::TransformComponent>(hudEntity);
      auto& hud = registry.emplace<ecs::CombatantHudComponent>(hudEntity);
      hud.enabled = true;
      hud.targetEntity = id;
      hud.texturePath = config.hud.texturePath;
      hud.worldOffset = config.hud.worldOffset;
      hud.heightMeters = config.hud.heightMeters;
      hud.tint = config.hud.tint;
      hud.maxRenderDistanceMeters = config.hud.maxRenderDistanceMeters;
      hud.distanceScaleEnabled = config.hud.distanceScaleEnabled;
      hud.distanceScaleStartMeters = config.hud.distanceScaleStartMeters;
      hud.distanceScaleEndMeters = config.hud.distanceScaleEndMeters;
      hud.distanceScaleAtEnd = config.hud.distanceScaleAtEnd;
      hud.fillEnabled = config.hud.fillEnabled;
      hud.fillColor = config.hud.fillColor;
      hud.fillWidthRatio = config.hud.fillWidthRatio;
      hud.fillHeightRatio = config.hud.fillHeightRatio;
      hud.fillDepthBiasMeters = config.hud.fillDepthBiasMeters;
    }
  }

  if (config.playerHud.enabled) {
    const EntityId hudEntity = registry.createEntity("player_hud");
    if (hudEntity != kInvalidEntityId) {
      auto& hud = registry.emplace<ecs::PlayerHudComponent>(hudEntity);
      hud.enabled = true;
      hud.targetEntity = id;
      hud.texturePath = config.playerHud.texturePath;
      hud.heightPx = config.playerHud.heightPx;
      hud.marginLeftPx = config.playerHud.marginLeftPx;
      hud.marginBottomPx = config.playerHud.marginBottomPx;
      hud.tint = config.playerHud.tint;
      hud.flipU = config.playerHud.flipU;
      hud.flipV = config.playerHud.flipV;
      hud.fillEnabled = config.playerHud.fillEnabled;
      hud.fillLayer = config.playerHud.fillLayer;
      hud.fillColor = config.playerHud.fillColor;
      hud.fillWidthRatio = config.playerHud.fillWidthRatio;
      hud.fillHeightRatio = config.playerHud.fillHeightRatio;
      hud.fillOffsetPx = config.playerHud.fillOffsetPx;
      hud.fillFromRight = config.playerHud.fillFromRight;
    }
  }

  return id;
}

}  // namespace ecs::services
