#include "ecs/systems/CombatInteractionSystem.h"

#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/CombatVolumeComponent.h"
#include "ecs/components/SocketComponent.h"
#include "ecs/events/CollisionEvents.h"
#include "ecs/events/CombatEvents.h"
#include "ecs/events/RaycastEvents.h"

namespace {

ecs::EntityId ownerForEntity(ecs::EntityRegistry& registry, ecs::EntityId entity) {
  if (entity == ecs::kInvalidEntityId || !registry.isAlive(entity)) return ecs::kInvalidEntityId;
  if (const auto* combat = registry.tryGet<ecs::CombatVolumeComponent>(entity)) {
    if (registry.isAlive(combat->ownerEntity)) return combat->ownerEntity;
  }
  if (const auto* socket = registry.tryGet<ecs::SocketComponent>(entity)) {
    if (registry.isAlive(socket->targetEntity)) return socket->targetEntity;
  }
  if (const auto* attach = registry.tryGet<ecs::AttachmentComponent>(entity)) {
    for (const auto& a : attach->attachments) {
      if (a.enabled && registry.isAlive(a.targetEntity)) return a.targetEntity;
    }
  }
  return entity;
}

bool matchesEntities(ecs::EntityId a, ecs::EntityId b, ecs::EntityId first, ecs::EntityId second) {
  return (a == first && b == second) || (a == second && b == first);
}

}  // namespace

namespace ecs::systems {

void CombatInteractionSystem::tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) const {
  (void)timeSeconds;
  const auto contacts = events.consumeAll<ecs::events::CombatContactDetectEvent>();
  const auto& alerts = events.peekAll<ecs::events::SensorAlertEvent>();
  const auto& collisions = events.peekAll<ecs::events::CollisionResolutionEvent>();

  for (const auto& contact : contacts) {
    const ecs::EntityId hitOwner = ownerForEntity(registry, contact.hitEntity);
    const ecs::EntityId hurtOwner = ownerForEntity(registry, contact.hurtEntity);

    bool hadRaycastSupport = false;
    for (const auto& alert : alerts) {
      const ecs::EntityId alertHitOwner = ownerForEntity(registry, alert.hitEntity);
      if (matchesEntities(alert.parentEntity, alertHitOwner, hitOwner, hurtOwner) ||
          matchesEntities(alert.parentEntity, alert.hitEntity, hitOwner, contact.hurtEntity)) {
        hadRaycastSupport = true;
        break;
      }
    }

    bool hadPhysicsCollision = false;
    for (const auto& collision : collisions) {
      const ecs::EntityId ownerA = ownerForEntity(registry, collision.entityA);
      const ecs::EntityId ownerB = ownerForEntity(registry, collision.entityB);
      if (matchesEntities(collision.entityA, collision.entityB, contact.hitEntity, contact.hurtEntity) ||
          matchesEntities(ownerA, ownerB, hitOwner, hurtOwner)) {
        hadPhysicsCollision = true;
        break;
      }
    }

    events.emit<ecs::events::CombatImpactEvent>({contact.hitEntity,
                                                 contact.hurtEntity,
                                                 hitOwner,
                                                 hurtOwner,
                                                 contact.hitVolumeIndex,
                                                 contact.hurtVolumeIndex,
                                                 true,
                                                 hadRaycastSupport,
                                                 hadPhysicsCollision,
                                                 contact.contactPoint,
                                                 contact.contactNormal,
                                                 contact.penetrationDepth,
                                                 contact.damage,
                                                 contact.force,
                                                 contact.damageType,
                                                 contact.time});

    if (contact.force > 0.0001f) {
      const ecs::EntityId knockbackTarget = hurtOwner != ecs::kInvalidEntityId ? hurtOwner : contact.hurtEntity;
      events.emit<ecs::events::KnockbackEvent>(
          {hitOwner != ecs::kInvalidEntityId ? hitOwner : contact.hitEntity,
           knockbackTarget,
           contact.contactNormal,
           contact.contactPoint,
           contact.force,
           hadRaycastSupport,
           hadPhysicsCollision,
           contact.time});
    }
  }
}

}  // namespace ecs::systems
