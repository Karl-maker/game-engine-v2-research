#pragma once

// Author: Karl-Johan Bailey
//
// PersonFactory
// Spawns a "person" style entity with physics (rigidbody + collider) and optional visuals.

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/factories/FactoryInputs.h"

namespace ecs::services {

struct ActorConfig final {
  TransformInput transform{.name = "person"};
  ViewableInput viewable{};
  PhysicalInput physical{};
  SkeletonInput skeleton{};
  StatsInput stats{};
  AnimationInput animation{};
  PoseInput pose{};
  IkInput ik{};
  SensorConeInput sensorCone{};
};

class ActorFactory final {
 public:
  EntityId create(EntityRegistry& registry, const ActorConfig& config);
};

}  // namespace ecs::services
