#pragma once

// Author: Karl-Johan Bailey
//
// FactoryInputs
// Reusable "mix and match" config blocks for factories.

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/EntityId.h"
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

struct GrassLayerInput final {
  std::string species = "BillboardGrassPlanes";
  std::string description =
      "Camera-relative crossed billboard grass planes with distance-based LOD and texture variation.";

  float density = 5.6f;
  float minScale = 0.54f;
  float maxScale = 0.92f;

  float bladeSpacing = 0.56f;
  float bendStrength = 0.18f;
  float curveStrength = 0.22f;
  float twistStrength = 0.10f;

  float minSlopeDeg = 0.0f;
  float maxSlopeDeg = 42.0f;
  float minAltitude = -10000.0f;
  float maxAltitude = 10000.0f;

  float noiseScale = 0.055f;
  float noiseStrength = 0.54f;

  float windStrength = 0.0f;
  float maxDistance = 34.0f;
};

struct GrassInput final {
  bool enabled = true;

  math::Vec3 area{14.0f, 0.0f, 14.0f};
  float densityMultiplier = 1.25f;
  std::uint32_t seed = 12345u;

  terrain::NoiseConfig densityNoise{.type = terrain::NoiseType::Perlin,
                                    .seed = 12345u,
                                    .frequency = 0.03f,
                                    .octaves = 3,
                                    .lacunarity = 2.0f,
                                    .persistence = 0.55f};
  float densityNoiseThreshold = 0.42f;
  float densityNoiseContrast = 3.0f;
  float densityNoiseStrength = 1.0f;

  terrain::NoiseConfig islandNoise{.type = terrain::NoiseType::Perlin,
                                   .seed = 4242u,
                                   .frequency = 0.045f,
                                   .octaves = 3,
                                   .lacunarity = 2.0f,
                                   .persistence = 0.58f};
  math::Vec3 islandNoiseOffset{0.0f, 0.0f, 0.0f};
  float islandNoiseThreshold = 0.44f;
  float islandNoiseSoftness = 0.18f;
  float islandNoiseContrast = 1.2f;
  float islandNoiseStrength = 1.0f;

  // Optional. If invalid, the grass factory can auto-bind a terrain at the same coord.
  EntityId sourceTerrainEntity = kInvalidEntityId;
  bool autoBindTerrain = true;

  std::vector<GrassLayerInput> layers = {GrassLayerInput{}};

  bool interactionEnabled = false;
  float interactionRadiusMeters = 1.25f;
  float interactionStrength = 1.0f;

  bool castShadows = false;
  bool receiveShadows = true;
  float lodBias = 1.0f;

  // Shader config for grass rendering (required by the graphics system).
  bool hasShader = true;
  std::string shaderKey = "graphics/shaders/grass";
  bool doubleSided = true;
  bool depthWrite = true;
  std::vector<render::TextureBinding> textures;
  std::vector<render::MaterialParameter> parameters;
  std::vector<ShaderBreakpointInput> lodBreakpoints;
};

}  // namespace ecs::services
