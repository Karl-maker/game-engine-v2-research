#pragma once

// Author: Karl-Johan Bailey
//
// FactoryInputs
// Reusable "mix and match" config blocks for factories.

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/MeshComponent.h"
#include "math/Vec3.h"
#include "physics/LayerMask.h"

#include <string>
#include <vector>

namespace ecs::services {

struct TransformInput final {
  std::string name = "entity";
  math::Vec3 position{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationDeg{0.0f, 0.0f, 0.0f};
  math::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct ViewableInput final {
  // If `meshKey` is empty, no mesh/shader is attached.
  std::string meshId;
  std::string meshKey;
  ecs::MeshComponent::MeshType meshType = ecs::MeshComponent::MeshType::Static;
  math::Vec3 meshScale{1.0f, 1.0f, 1.0f};
  std::string skeletonId;
  bool visible = true;
  bool castShadows = true;
  bool receiveShadows = true;
  std::vector<std::string> tags;

  // Shader asset key (engine-defined).
  std::string shaderKey = "graphics/shaders/model";
};

struct PhysicalInput final {
  // Rigidbody.
  bool hasRigidbody = true;
  float mass = 80.0f;
  bool useGravity = true;
  bool kinematic = false;

  // Collider.
  bool hasCollider = true;
  ecs::ColliderComponent::Shape colliderShape = ecs::ColliderComponent::Shape::Capsule;
  math::Vec3 colliderSize{0.38f, 1.85f, 0.38f};   // capsule: (radius, height, _)
  math::Vec3 colliderOffset{0.0f, 0.925f, 0.0f};  // centered for capsule height above ground
  bool colliderIsTrigger = false;
  physics::LayerMask collisionLayer = physics::kLayerCharacter;
};

struct SkeletonInput final {
  std::string skeletonData;
};

struct StatsInput final {
  float maxHealth = 100.0f;
  float health = maxHealth;

  float defense = 20.0f;
  float baseAttack = 20.0f;
  float specialAttack = 0.0f;

  float speed = 20.0f;

  float walkingSpeed = 10.0f;
  float runningSpeed = 20.0f;
  float swimmingSpeed = 10.0f;
};

}  // namespace ecs::services

