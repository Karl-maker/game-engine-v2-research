// Author: Karl-Johan Bailey

#include "BuildingFactory.h"

namespace ecs::services {

EntityId BuildingFactory::create(
    EntityRegistry& registry,
    const BuildingConfig& config) {

  // Create the root character entity.
  EntityId buildingEntity =
      registry.createEntity();

  if (buildingEntity == kInvalidEntityId) {
    return kInvalidEntityId;
  }

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

  return buildingEntity;
}

}  // namespace ecs::services