#pragma once

// Author: Karl-Johan Bailey
//
// FactoryInputs
// Reusable "mix and match" config blocks for factories.

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/CombatVolumeComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/RaycastComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/BillboardComponent.h"
#include "ecs/components/VfxComponent.h"
#include "ecs/EntityId.h"
#include "render/AnimatedTexture.h"
#include "render/MaterialParameter.h"
#include "render/TextureBinding.h"
#include "render/Color.h"
#include "math/Mat4.h"
#include "math/Vec2.h"
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

struct BillboardInput final {
  TransformInput transform{.name = "billboard"};
  bool enabled = true;
  bool visible = true;
  bool textureEnabled = true;
  bool depthWrite = false;
  bool doubleSided = true;
  ecs::BillboardComponent::FaceMode faceMode = ecs::BillboardComponent::FaceMode::CameraPlane;
  math::Vec2 sizeMeters{1.25f, 1.25f};
  math::Vec2 pivot{0.5f, 0.0f};
  math::Vec3 worldOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationOffsetDeg{0.0f, 0.0f, 0.0f};
  float maxRenderDistance = 96.0f;
  render::AssetRef texture{};
  render::AnimatedTexture animatedTexture{};
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
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
  std::string colliderMeshId;
  std::string colliderMeshKey;
  bool colliderUseMeshBounds = false;
  bool buoyant = false;
  float buoyancyHeight = -0.5f;
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

struct CombatantHudInput final {
  bool enabled = true;
  std::string texturePath = "assets/hud/combatant-health.png";
  math::Vec3 worldOffset{0.0f, 2.25f, 0.0f};
  float heightMeters = 0.22f;
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};

  // Visibility/culling.
  float maxRenderDistanceMeters = 180.0f;

  // Subtle distance scaling (makes the bar slightly larger when further away).
  bool distanceScaleEnabled = true;
  float distanceScaleStartMeters = 8.0f;
  float distanceScaleEndMeters = 42.0f;
  float distanceScaleAtEnd = 1.20f;

  // Optional dynamic fill quad drawn in front.
  bool fillEnabled = true;
  render::Color fillColor{0.10f, 0.95f, 0.25f, 0.85f};
  float fillWidthRatio = 0.84f;   // fraction of base width usable for fill
  float fillHeightRatio = 0.28f;  // fraction of base height usable for fill
  float fillDepthBiasMeters = 0.015f;
};

struct PlayerHudInput final {
  bool enabled = true;
  std::string texturePath = "assets/hud/player-health.png";
  float heightPx = 96.0f;
  float marginLeftPx = 24.0f;
  float marginBottomPx = 24.0f;
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};

  bool flipU = false;
  bool flipV = false;

  bool fillEnabled = true;
  int fillLayer = 1;
  render::Color fillColor{0.10f, 0.95f, 0.25f, 0.90f};
  float fillWidthRatio = 0.84f;
  float fillHeightRatio = 0.28f;
  math::Vec2 fillOffsetPx{24.0f, 28.0f};
  bool fillFromRight = false;
};

struct AnimationLayerInput final {
  std::string name = "Base Layer";
  float weight = 1.0f;
  std::string blendMode = "Override";  // "Override" or "Additive"
  std::vector<std::string> mask;
  std::string currentState = "Idle";
  std::string nextState;
  float transition = 0.0f;
};

struct AnimationClipBindingInput final {
  std::string key;
  std::string clip;
  float speed = 1.0f;
};

struct AnimationInput final {
  bool enabled = false;
  std::vector<std::string> availableClips = {"IdleV4.2(maya_head)", "Idle", "Walk", "Run"};
  std::vector<AnimationClipBindingInput> clipBindings;
  std::vector<AnimationLayerInput> layers = {AnimationLayerInput{.currentState = "IdleV4.2(maya_head)"}};
  float idleDelaySeconds = 5.0f;
  std::string idleAnimationClip = "IdleV4.2(maya_head)";
  std::string idleAnimationKey = "idle";
  std::string idleAnimationLayer = "Base Layer";
  std::string locomotionIdleKey = "idle";
  std::string locomotionWalkKey = "walk";
  std::string locomotionRunKey = "run";
};

struct PoseBoneOverrideInput final {
  std::string boneKey;
  float weight = 1.0f;
  bool hasTranslation = false;
  math::Vec3 translation{0.0f, 0.0f, 0.0f};
  bool hasRotationEulerDeg = false;
  math::Vec3 rotationEulerDeg{0.0f, 0.0f, 0.0f};
  bool hasRotationQuat = false;
  math::Quat rotation{};
  bool hasScale = false;
  math::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct PoseDefinitionInput final {
  std::string name;
  bool enabled = true;
  float weight = 0.0f;
  std::vector<PoseBoneOverrideInput> bones;
};

struct PoseInput final {
  bool enabled = false;
  std::string defaultPoseName = "right_hand_pose";
  bool defaultPoseEnabled = true;
  float defaultPoseWeight = 0.0f;
  std::vector<PoseDefinitionInput> poses;
};

struct IkChainInput final {
  bool enabled = true;
  std::string name;
  std::vector<std::string> bones;

  // Optional entity target for initialization.
  EntityId targetEntity = kInvalidEntityId;
  math::Vec3 targetOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 targetLocalOffset{0.0f, 0.0f, 0.0f};

  float weight = 1.0f;
  float blendInSeconds = 0.25f;
  float blendOutSeconds = 0.20f;
  int iterations = 8;
  bool overrideAnimation = true;
};

struct IkInput final {
  bool enabled = false;

  // Defaults match the current demo skeleton.
  IkChainInput headLook{
      .enabled = true,
      .name = "look_at_camera",
      .bones = {"Neck_7", "Head_6"},
      .targetEntity = kInvalidEntityId,
      .targetOffset = {0.0f, -0.10f, 0.0f},
      .weight = 1.0f,
      .blendInSeconds = 0.25f,
      .blendOutSeconds = 0.20f,
      .iterations = 6,
      .overrideAnimation = true,
  };

  IkChainInput reachTarget{
      .enabled = true,
      .name = "reach_seen_target",
      .bones = {"RightArm_44", "RightForeArm_43", "RightHand_42"},
      .targetEntity = kInvalidEntityId,
      .targetOffset = {0.0f, 1.2f, 0.15f},
      .weight = 0.92f,
      .blendInSeconds = 0.25f,
      .blendOutSeconds = 0.20f,
      .iterations = 8,
      .overrideAnimation = true,
  };
};

struct RaycastConeInput final {
  int rayCount = 12;
  float coneAngleDeg = 22.0f;
  float length = 16.0f;
  float radius = 0.0f;
  physics::LayerMask collisionLayers = physics::kLayerCharacter;
  physics::LayerMask ignoreLayers = 0;
  bool ignoreSelf = true;
  int maxHits = 1;
  math::Vec3 originLocalOffset{0.0f, 0.0f, 0.0f};
  std::string baseName = "player_head_ray";
};

struct SensorConeInput final {
  bool enabled = false;
  std::string sensorName = "player_head_sensor";
  std::string socketName = "player_head_socket";
  math::Vec3 socketPositionOffset{0.0f, 0.0f, 0.0f};
  RaycastConeInput cone{};
};

struct AttachmentMountInput final {
  std::string mode = "parent";
  EntityId targetEntity = kInvalidEntityId;
  std::string targetEntityName;
  std::string targetMeshId;
  std::string skeletonId;
  std::string boneName;
  std::string socketName;
  math::Vec3 positionOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 scaleOffset{0.0f, 0.0f, 0.0f};
  bool inheritPosition = true;
  bool inheritRotation = true;
  bool inheritScale = false;
};

struct CombatAttachmentInput final {
  std::string name = "combat_volume";
  std::string sourceMeshId;
  AttachmentMountInput mount{};
  std::vector<ecs::CombatVolumeComponent::Volume> volumes;
};

struct RaycastAttachmentInput final {
  std::string name = "raycast";
  std::string sensorName;
  bool createSensor = false;
  AttachmentMountInput mount{};
  ecs::RaycastComponent raycast{};
};

struct VfxAttachmentInput final {
  std::string name = "vfx";
  AttachmentMountInput mount{};
  ecs::VfxComponent vfx{};
};

struct CombatSetupInput final {
  std::vector<CombatAttachmentInput> volumes;
  std::vector<RaycastAttachmentInput> raycasts;
  std::vector<VfxAttachmentInput> vfx;
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
