// Author: Karl-Johan Bailey

#include "EnemyFactory.h"
#include "ecs/components/TransformComponent.h"

namespace ecs::services {

EntityId EnemyFactory::create(
    EntityRegistry& registry,
    const EnemyConfig& config) {

  // Create the root character entity.
  EntityId enemyEntity =
      registry.createEntity();

  if (enemyEntity == kInvalidEntityId) {
    return kInvalidEntityId;
  }

  auto& playerTr = registry.emplace<ecs::TransformComponent>(enemyEntity);

  // ------------------------------------------------------------
  // Identity
  // ------------------------------------------------------------

  // Create and configure the character's identity component.
  // registry.emplace<IdentityComponent>(characterEntity);

  // ------------------------------------------------------------
  // Transform
  // ------------------------------------------------------------

  // Create the character transform.
  // registry.emplace<TransformComponent>(characterEntity);

  // ------------------------------------------------------------
  // Mesh
  // ------------------------------------------------------------

  // Create/configure the character mesh.
  // registry.emplace<MeshComponent>(characterEntity);

  // ------------------------------------------------------------
  // Skeleton
  // ------------------------------------------------------------

  // Create/configure the character skeleton.
  // registry.emplace<SkeletonComponent>(characterEntity);

  // ------------------------------------------------------------
  // Animation
  // ------------------------------------------------------------

  // Create/configure the animation component.
  // registry.emplace<AnimationComponent>(characterEntity);

  // ------------------------------------------------------------
  // Character Controller
  // ------------------------------------------------------------

  // Create/configure the character controller.
  // registry.emplace<CharacterControllerComponent>(characterEntity);

  // ------------------------------------------------------------
  // Motion
  // ------------------------------------------------------------

  // Create/configure the character motion component.
  // registry.emplace<MotionComponent>(characterEntity);

  // ------------------------------------------------------------
  // Return the fully constructed character.
  // ------------------------------------------------------------

  return enemyEntity;
}

}  // namespace ecs::services