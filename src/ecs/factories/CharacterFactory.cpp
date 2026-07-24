// Author: Karl-Johan Bailey

#include "CharacterFactory.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/ColliderComponent.h"

namespace ecs::services {

EntityId CharacterFactory::create(
    EntityRegistry& registry,
    const CharacterConfig& config) {

  // Create the root character entity.
  EntityId characterEntity =
      registry.createEntity();

  if (characterEntity == kInvalidEntityId) {
    return kInvalidEntityId;
  }

  {   
    auto& transform = registry.emplace<ecs::TransformComponent>(characterEntity);
  }
  {
    auto& motion = registry.emplace<ecs::MotionComponent>(characterEntity);
    motion.mode = ecs::MotionComponent::Mode::Walking;
    motion.isGrounded = false;
  }
  {
    auto& body = registry.emplace<ecs::RigidbodyComponent>(characterEntity);
    body.mass = 80.0f;
    body.inverseMass = 1.0f / body.mass;
    body.useGravity = true;
  }
  {
    auto& collider = registry.emplace<ecs::ColliderComponent>(characterEntity);
    collider.shape = ecs::ColliderComponent::Shape::Capsule;
    collider.size = {0.38f, 1.85f, 0.38f};
    collider.offset = {0.0f, 0.925f, 0.0f};
    collider.collisionLayer = physics::kLayerCharacter;
  }
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

  return characterEntity;
}

}  // namespace ecs::services