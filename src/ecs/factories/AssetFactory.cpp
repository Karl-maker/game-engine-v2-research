// Author: Karl-Johan Bailey

/**
 * Optionally make it rigid or not / dynamic. 
 *  - Make it breakable or movable
 */

#include "AssetFactory.h"

namespace ecs::services {

EntityId AssetFactory::create(
    EntityRegistry& registry,
    const AssetConfig& config) {

  // Create the root character entity.
  EntityId assetEntity =
      registry.createEntity();

  if (assetEntity == kInvalidEntityId) {
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

  return assetEntity;
}

}  // namespace ecs::services