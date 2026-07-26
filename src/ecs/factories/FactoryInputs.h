#pragma once

// Author: Karl-Johan Bailey
//
// FactoryInputs
// Reusable "mix and match" config blocks for factories.

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "render/MaterialParameter.h"
#include "render/TextureBinding.h"
#include "math/Vec3.h"
#include "physics/LayerMask.h"
#include "terrain/NoiseConfig.h"

#include <string>
#include <vector>

namespace ecs::services {

struct TransformInput final {
  std::string name = "entity";
  math::Vec3 position{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationDeg{0.0f, 0.0f, 0.0f};
  math::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct ShaderBreakpointInput final {
  float distanceMeters = 0.0f;
  std::vector<render::TextureBinding> textures;
  std::vector<render::MaterialParameter> parameters;
  bool overrideTessellation = false;
  float tessNear = 0.0f;
  float tessFar = 0.0f;
  float tessMin = 0.0f;
  float tessMax = 0.0f;
  int tessQuality = 0;
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
  std::vector<render::TextureBinding> textures;
  std::vector<render::MaterialParameter> parameters;
  std::vector<ShaderBreakpointInput> lodBreakpoints;
};

struct TerrainInput final {
  TransformInput transform{.name = "terrain"};
  bool enabled = true;
  int gridWidth = 512;
  int gridHeight = 512;
  float cellSizeMeters = 1.0f;
  float heightScaleMeters = 150.0f;
  std::uint32_t noiseSeed = 12345u;
  terrain::NoiseConfig noise{};
  float lodMaxRenderDistance = 240.0f;
  float lodStep1Distance = 24.0f;
  float lodStep2Distance = 48.0f;
  float lodStep4Distance = 84.0f;
  float lodStep8Distance = 132.0f;
  float lodStep16Distance = 180.0f;
  float lodForceNearDistance = 18.0f;
  float tessLockDistance = 16.0f;
  float tessEnableDistance = 72.0f;
  float tessDisableDistance = 112.0f;
  float viewDotBias = 0.05f;
  bool hasCollider = true;
  float colliderThicknessMeters = 5.0f;
  physics::LayerMask collisionLayer = physics::kLayerWorld;
  bool hasShader = true;
  ViewableInput viewable{};
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
