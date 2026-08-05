#pragma once

// Author: Karl-Johan Bailey
//
// PersonFactory
// Spawns a "person" style entity with physics (rigidbody + collider) and optional visuals.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct CombatantConfig final {
  CombatantConfig() { playerHud.enabled = false; }

  TransformInput transform{.name = "combatant"};
  ViewableInput viewable{};
  PhysicalInput physical{};
  SkeletonInput skeleton{};
  StatsInput stats{};
  AnimationInput animation{};
  PoseInput pose{};
  IkInput ik{};
  SensorConeInput sensorCone{};
  CombatSetupInput combat{};
  CombatantHudInput hud{};
  PlayerHudInput playerHud{};
};

class CombatantFactory final {
 public:
  EntityId create(EntityRegistry& registry, const CombatantConfig& config);
};

}  // namespace ecs::services
