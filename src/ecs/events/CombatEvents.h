#pragma once

// Author: Karl-Johan Bailey
//
// Combat contact events are ephemeral. The combat hit detection system emits them when hit and hurt
// volumes overlap while the hit volume is active.

#include "ecs/EntityId.h"
#include "math/Vec3.h"

#include <string>

namespace ecs::events {

struct CombatContactDetectEvent final {
  ecs::EntityId hitEntity = ecs::kInvalidEntityId;
  ecs::EntityId hurtEntity = ecs::kInvalidEntityId;
  int hitVolumeIndex = -1;
  int hurtVolumeIndex = -1;
  math::Vec3 contactPoint{};
  math::Vec3 contactNormal{0.0f, 1.0f, 0.0f};
  float penetrationDepth = 0.0f;
  float damage = 0.0f;
  float force = 0.0f;
  std::string damageType;
  double time = 0.0;
};

struct CombatImpactEvent final {
  ecs::EntityId hitEntity = ecs::kInvalidEntityId;
  ecs::EntityId hurtEntity = ecs::kInvalidEntityId;
  ecs::EntityId hitOwnerEntity = ecs::kInvalidEntityId;
  ecs::EntityId hurtOwnerEntity = ecs::kInvalidEntityId;
  int hitVolumeIndex = -1;
  int hurtVolumeIndex = -1;
  bool hadVolumeContact = false;
  bool hadRaycastSupport = false;
  bool hadPhysicsCollision = false;
  math::Vec3 contactPoint{};
  math::Vec3 contactNormal{0.0f, 1.0f, 0.0f};
  float penetrationDepth = 0.0f;
  float damage = 0.0f;
  float force = 0.0f;
  std::string damageType;
  double time = 0.0;
};

struct KnockbackEvent final {
  ecs::EntityId sourceEntity = ecs::kInvalidEntityId;
  ecs::EntityId targetEntity = ecs::kInvalidEntityId;
  math::Vec3 direction{0.0f, 0.0f, 0.0f};
  math::Vec3 contactPoint{};
  float strength = 0.0f;
  bool hadRaycastSupport = false;
  bool hadPhysicsCollision = false;
  double time = 0.0;
};

}  // namespace ecs::events
