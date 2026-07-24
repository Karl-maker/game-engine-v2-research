#include "games/GameplayDemoGame.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"

#include "ecs/components/CameraComponent.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/CharacterComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/CombatVolumeComponent.h"
#include "ecs/components/ControllerComponent.h"
#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/IKComponent.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/RenderSettingsComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/SensorComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/GrassPatchComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/SocketComponent.h"
#include "ecs/components/ThirdPersonCameraComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/RaycastEvents.h"
#include "ecs/services/IKService.h"
#include "ecs/services/RaycastConeFactoryService.h"
#include "materials/presets/HighQualityDirtRockLayer.h"
#include "materials/presets/StoneGrass.h"
#include "materials/presets/HighQualityDirtRockGrassLayer.h"
#include "materials/presets/RealisticSkyClouds.h"

#include <algorithm>
#include <iostream>

namespace games {

namespace {

struct VisionDebugResult final {
  bool sawObservedTarget = false;
  std::string firstHitName;
};

math::Vec3 forwardFromPitchYawDeg(float pitchDeg, float yawDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kDegToRad = kPi / 180.0f;
  const float pitch = pitchDeg * kDegToRad;
  const float yaw = yawDeg * kDegToRad;
  return math::Vec3{std::cos(pitch) * std::sin(yaw), -std::sin(pitch), std::cos(pitch) * std::cos(yaw)};
}

void configurePlayerHeadIk(ecs::EntityRegistry& registry, ecs::EntityId actor, ecs::EntityId target) {
  auto* ik = registry.tryGet<ecs::IKComponent>(actor);
  if (!ik) return;

  auto& headChain = ecs::services::IKService::ensureChain(*ik, "look_at_camera", {"Neck_7", "Head_6"});
  headChain.overrideAnimation = true;
  ecs::services::IKService::setEntityTarget(headChain, target, {0.0f, -0.10f, 0.0f});
  ecs::services::IKService::setWeight(headChain, 1.0f);
  ecs::services::IKService::setBlendTimes(headChain, 0.25f, 0.20f);
  ecs::services::IKService::setIterations(headChain, 6);
}

void updatePlayerHeadFacingIk(ecs::EntityRegistry& registry, ecs::EntityId actor, ecs::EntityId camera) {
  auto* ik = registry.tryGet<ecs::IKComponent>(actor);
  if (!ik || camera == ecs::kInvalidEntityId) return;

  ecs::IKComponent::Chain* headChain = nullptr;
  for (auto& chain : ik->chains) {
    if (chain.name == "look_at_camera") {
      headChain = &chain;
      break;
    }
  }
  if (!headChain || !registry.isAlive(camera)) return;

  const auto* camTr = registry.tryGet<ecs::TransformComponent>(camera);
  if (!camTr) return;

  const math::Vec3 forward = forwardFromPitchYawDeg(camTr->rotation.x, camTr->rotation.y);
  ecs::services::IKService::setWorldTarget(*headChain, camTr->position + forward * 20.0f);
  headChain->enabled = true;
}

void configurePlayerHandReachIk(ecs::EntityRegistry& registry, ecs::EntityId actor, ecs::EntityId target) {
  auto* ik = registry.tryGet<ecs::IKComponent>(actor);
  if (!ik) return;

  auto& handChain =
      ecs::services::IKService::ensureChain(*ik, "reach_seen_target", {"RightArm_44", "RightForeArm_43", "RightHand_42"});
  handChain.overrideAnimation = true;
  handChain.enabled = false;
  handChain.targetLocalOffset = {0.0f, 0.0f, 0.0f};
  ecs::services::IKService::setEntityTarget(handChain, target, {0.0f, 1.2f, 0.15f});
  ecs::services::IKService::setWeight(handChain, 0.92f);
  ecs::services::IKService::setBlendTimes(handChain, 0.25f, 0.20f);
  ecs::services::IKService::setIterations(handChain, 8);
}

VisionDebugResult updatePlayerVisionDrivenIk(ecs::EntityRegistry& registry,
                                             ecs::services::EventService& events,
                                             ecs::EntityId player,
                                             ecs::EntityId observedTarget) {
  VisionDebugResult result;
  auto* ik = registry.tryGet<ecs::IKComponent>(player);
  if (!ik) return result;

  ecs::IKComponent::Chain* reachChain = nullptr;
  for (auto& chain : ik->chains) {
    if (chain.name == "reach_seen_target") {
      reachChain = &chain;
      break;
    }
  }
  if (!reachChain) return result;

  bool targetSeen = false;
  bool hasTarget = false;
  math::Vec3 targetWorld{};
  const auto alerts = events.consumeAll<ecs::events::SensorAlertEvent>();
  for (const auto& alert : alerts) {
    if (alert.parentEntity != player) continue;
    if (result.firstHitName.empty()) {
      if (const auto* identity = registry.tryGet<ecs::IdentityComponent>(alert.hitEntity)) {
        result.firstHitName = identity->name;
      } else {
        result.firstHitName = "entity_" + std::to_string(alert.hitEntity);
      }
    }
    if (!hasTarget) {
      targetWorld = alert.hit.hitPosition;
      hasTarget = true;
    }
    if (alert.hitEntity == observedTarget) {
      targetSeen = true;
    }
  }

  reachChain->enabled = hasTarget;
  if (hasTarget) {
    ecs::services::IKService::setWorldTarget(*reachChain, targetWorld);
  }
  result.sawObservedTarget = targetSeen;
  return result;
}

bool hasActionRequest(const ecs::ControllerComponent* controller, const std::string& action) {
  if (!controller) return false;
  for (const auto& req : controller->actionRequests) {
    if (req.pressed && req.action == action) return true;
  }
  return false;
}

}  // namespace

void GameplayDemoGame::onStart() {
  std::cout << "\x1B[2J\x1B[H";
  std::cout << "Gameplay demo (graphics snapshot)\n";
  std::cout << "- Creates camera + terrain(shader) + light\n";
  std::cout << "- Camera is driven by Controller/Motion/Movement systems\n";
  std::cout << "Controls (type then Enter): w/a/s/d, move x y z, look pitch yaw, sprint/walk/crouch/jump, stop\n";
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  std::cout << "- Opens an OpenGL window and renders the terrain\n";
#else
  std::cout << "- Runs GraphicsSystem each tick (prints a snapshot)\n";
#endif
  std::cout << "Type `q` then Enter to quit.\n\n";

  m_meshAssets.start();

  m_camera = m_registry.createEntity("camera");
  auto& camTr = m_registry.emplace<ecs::TransformComponent>(m_camera);
  camTr.position = {0.0f, 3.0f, -6.0f};
  camTr.rotation = {12.0f, 0.0f, 0.0f};  // pitch/yaw/roll (deg)
  m_registry.emplace<ecs::CameraComponent>(m_camera);

  m_player = m_registry.createEntity("business_man");
  auto& playerTr = m_registry.emplace<ecs::TransformComponent>(m_player);
  playerTr.position = {0.0f, 0.0f, 0.0f};
  playerTr.rotation = {0.0f, 0.0f, 0.0f};
  m_registry.emplace<ecs::ControllerComponent>(m_player);
  m_registry.emplace<ecs::CharacterComponent>(m_player);
  {
    auto& motion = m_registry.emplace<ecs::MotionComponent>(m_player);
    motion.mode = ecs::MotionComponent::Mode::Walking;
    motion.isGrounded = false;
  }
  {
    auto& body = m_registry.emplace<ecs::RigidbodyComponent>(m_player);
    body.mass = 80.0f;
    body.inverseMass = 1.0f / body.mass;
    body.useGravity = true;
  }
  {
    auto& collider = m_registry.emplace<ecs::ColliderComponent>(m_player);
    collider.shape = ecs::ColliderComponent::Shape::Capsule;
    collider.size = {0.38f, 1.85f, 0.38f};
    collider.offset = {0.0f, 0.925f, 0.0f};
    collider.collisionLayer = physics::kLayerCharacter;
  }
  {
    auto& stats = m_registry.emplace<ecs::StatsComponent>(m_player);
    stats.walkingSpeed = 1.8f;
    stats.runningSpeed = 7.0f;
  }
  {
    auto& mesh = m_registry.emplace<ecs::MeshComponent>(m_player);
    mesh.meshId = "business-man";
    mesh.meshData.enabled = true;
    mesh.meshData.key = "assets/models/business-man/scene.gltf";
    mesh.meshType = ecs::MeshComponent::MeshType::Skinned;
    mesh.scale = {1.25f, 1.25f, 1.25f};
    mesh.skeletonId = "business-man#skin0";
    mesh.castShadows = true;
    mesh.receiveShadows = true;
    mesh.tags = {"character", "player"};

    auto& shader = m_registry.emplace<ecs::ShaderComponent>(m_player);
    shader.shader.key = "graphics/shaders/model";
    shader.castShadows = true;
    shader.receiveShadows = true;
  }
  {
    auto& skeleton = m_registry.emplace<ecs::SkeletonComponent>(m_player);
    skeleton.skeletonId = "business-man#skin0";
    skeleton.skeletonData = "assets/models/business-man/scene.gltf";
    skeleton.updateMode = ecs::SkeletonComponent::UpdateMode::WhenVisible;
  }
  {
    auto& animation = m_registry.emplace<ecs::AnimationComponent>(m_player);
    animation.availableClips = {"IdleV4.2(maya_head)", "Idle", "Walk", "Run"};
    animation.layers.push_back({"Base Layer", 1.0f, ecs::AnimationComponent::BlendMode::Override, {}, "IdleV4.2(maya_head)", "", 0.0f});
    animation.idleAnimationClip = "IdleV4.2(maya_head)";
    animation.idleDelaySeconds = 5.0f;
  }
  {
    auto& ik = m_registry.emplace<ecs::IKComponent>(m_player);
    ik.enabled = true;
    configurePlayerHeadIk(m_registry, m_player, m_camera);
    configurePlayerHandReachIk(m_registry, m_player, ecs::kInvalidEntityId);
  }
  {
    auto& thirdPerson = m_registry.emplace<ecs::ThirdPersonCameraComponent>(m_camera);

    thirdPerson.target = m_player;

    // Look toward upper chest / shoulder area.
    thirdPerson.targetOffset = {0.0f, 1.8f, 0.0f};
    thirdPerson.distance = 1.7f;

    // Small vertical lift.
    thirdPerson.height = 0.50f;

    thirdPerson.pitchDeg = 5.0f;
    thirdPerson.minPitchDeg = -30.0f;
    thirdPerson.maxPitchDeg = 45.0f;

    thirdPerson.yawDeg = playerTr.rotation.y;
  }

  {
    ecs::services::RaycastConeFactoryService rayFactory;
    ecs::services::SensorConeConfig cfg;
    cfg.sensorName = "player_head_sensor";
    cfg.socketName = "player_head_socket";
    cfg.socketPositionOffset = {0.0f, 0.00f, 0.00f};
    cfg.cone.baseName = "player_head_ray";
    cfg.cone.rayCount = 12;
    cfg.cone.coneAngleDeg = 22.0f;
    cfg.cone.length = 16.0f;
    cfg.cone.radius = 0.0f;
    cfg.cone.collisionLayers = physics::kLayerCharacter;
    cfg.cone.ignoreLayers = 0;
    cfg.cone.ignoreSelf = true;
    cfg.cone.maxHits = 1;
    cfg.cone.originLocalOffset = {0.0f, 0.0f, 0.0f};
    rayFactory.createSensorCone(m_registry, m_player, cfg);
  }

  // A second character that just stands there.
  {
    const auto npc = m_registry.createEntity("business_man_npc");
    m_demoNpc = npc;
    auto& npcTr = m_registry.emplace<ecs::TransformComponent>(npc);
    npcTr.position = {3.25f, 0.0f, -8.0f};
    npcTr.rotation = {0.0f, 180.0f, 0.0f};

    auto& npcMesh = m_registry.emplace<ecs::MeshComponent>(npc);
    npcMesh.meshId = "business-man";
    npcMesh.meshData.enabled = true;
    npcMesh.meshData.key = "assets/models/business-man/scene.gltf";
    npcMesh.meshType = ecs::MeshComponent::MeshType::Skinned;
    npcMesh.scale = {1.25f, 1.25f, 1.25f};
    npcMesh.skeletonId = "business-man#skin0";
    npcMesh.castShadows = true;
    npcMesh.receiveShadows = true;

    auto& npcShader = m_registry.emplace<ecs::ShaderComponent>(npc);
    npcShader.shader.key = "graphics/shaders/model";
    npcShader.castShadows = true;
    npcShader.receiveShadows = true;

    m_registry.emplace<ecs::SkeletonComponent>(npc);
    m_registry.emplace<ecs::CharacterComponent>(npc);
    {
      auto& npcMotion = m_registry.emplace<ecs::MotionComponent>(npc);
      npcMotion.mode = ecs::MotionComponent::Mode::Walking;
      npcMotion.isGrounded = true;
      npcMotion.isMoving = false;
    }
    {
      auto& npcBody = m_registry.emplace<ecs::RigidbodyComponent>(npc);
      npcBody.mass = 90.0f;
      npcBody.inverseMass = 1.0f / npcBody.mass;
      npcBody.useGravity = true;
    }
    {
      auto& npcCollider = m_registry.emplace<ecs::ColliderComponent>(npc);
      npcCollider.shape = ecs::ColliderComponent::Shape::Capsule;
      npcCollider.size = {0.38f, 1.85f, 0.38f};
      npcCollider.offset = {0.0f, 0.925f, 0.0f};
      npcCollider.collisionLayer = physics::kLayerCharacter;
    }
    auto& npcAnim = m_registry.emplace<ecs::AnimationComponent>(npc);
    npcAnim.availableClips = {"IdleV4.2(maya_head)", "Idle", "Walk", "Run"};
    npcAnim.layers.push_back({"Base Layer", 1.0f, ecs::AnimationComponent::BlendMode::Override, {}, "Idle", "", 0.0f});
    npcAnim.idleAnimationClip = "IdleV4.2(maya_head)";
    npcAnim.idleDelaySeconds = 5.0f;
  }

  configurePlayerHandReachIk(m_registry, m_player, m_demoNpc);

  m_terrain = m_registry.createEntity("terrain");
  m_registry.emplace<ecs::TransformComponent>(m_terrain);
  {
    auto& terrain = m_registry.emplace<ecs::TerrainComponent>(m_terrain);
    terrain.gridWidth = 96;
    terrain.gridHeight = 96;
    terrain.cellSizeMeters = 1.0f;
    // Flatter terrain (less "mountainy").
    terrain.heightScaleMeters = 2.6f;
    terrain.noise.frequency = 0.030f;
    terrain.noise.octaves = 2;
    terrain.noise.persistence = 0.45f;
    terrain.noise.lacunarity = 2.0f;
    auto& collider = m_registry.emplace<ecs::ColliderComponent>(m_terrain);
    collider.shape = ecs::ColliderComponent::Shape::Terrain;
    collider.collisionLayer = physics::kLayerWorld;
    collider.terrain.enabled = true;
    collider.terrain.sourceTerrainEntity = m_terrain;
    collider.terrain.collisionLayer = physics::kLayerWorld;
    collider.terrain.thicknessMeters = 5.0f;

    auto& shader = m_registry.emplace<ecs::ShaderComponent>(m_terrain, materials::presets::HighQualityDirtRockLayer());
    // Render using the current OpenGL demo shader (textures are ignored for now).
    shader.shader.key = "graphics/shaders/terrain";
    shader.depthWrite = true;

    // Scene override: remove the circular "sink" patches (often mistaken for pebbles).
    shader.parameters.push_back({"dirtSinksEnabled", false});
  }

  // Grass patches (instanced clumps; layered for variation).
  {
    const ecs::EntityId grass = m_registry.createEntity("grass_patch");
    auto& tr = m_registry.emplace<ecs::TransformComponent>(grass);
    tr.position = {0.0f, 0.0f, 0.0f};

    auto& gp = m_registry.emplace<ecs::GrassPatchComponent>(grass);
    gp.sourceTerrainEntity = m_terrain;
    gp.area = {5.0f, 0.0f, 5.0f};
    gp.seed = 9001u;
    gp.densityMultiplier = 1.2f;
    gp.densityNoise.seed = 1777u;
    gp.densityNoise.frequency = 0.12f;
    gp.densityNoise.octaves = 2;
    gp.densityNoise.persistence = 0.50f;
    gp.densityNoise.lacunarity = 2.0f;
    gp.densityNoiseThreshold = 0.10f;
    gp.densityNoiseContrast = 0.72f;
    gp.densityNoiseStrength = 1.0f;
    gp.islandNoise.seed = 7331u;
    gp.islandNoise.frequency = 0.042f;
    gp.islandNoise.octaves = 3;
    gp.islandNoise.persistence = 0.62f;
    gp.islandNoise.lacunarity = 2.05f;
    gp.islandNoiseOffset = {0.0f, 0.0f, 0.0f};
    gp.islandNoiseThreshold = 0.40f;
    gp.islandNoiseSoftness = 0.20f;
    gp.islandNoiseContrast = 1.15f;
    gp.islandNoiseStrength = 0.92f;
    gp.interactionEnabled = false;
    gp.interactionRadiusMeters = 1.0f;
    gp.interactionStrength = 1.0f;

    // Billboard grass planes: three intersecting planes nearby, cheaper LOD farther out.
    gp.layers = {
        ecs::GrassPatchComponent::GrassLayer{.species = "BillboardGrassPlanes",
                                             .description =
                                                 "Three off-center intersecting billboard planes with random heights and texture variation.",
                                             .density = 4.8f,
                                             .minScale = 0.70f,
                                             .maxScale = 1.18f,
                                             .bladeSpacing = 1.70f,
                                             .bendStrength = 0.16f,
                                             .curveStrength = 0.16f,
                                             .twistStrength = 0.0f,
                                             .noiseScale = 0.52f,
                                             .noiseStrength = 1.56f,
                                             .windStrength = 0.0f,
                                             .maxDistance = 42.0f},
    };

    auto& sh = m_registry.emplace<ecs::ShaderComponent>(grass);
    sh.shader.key = "graphics/shaders/grass_planes";
    sh.doubleSided = true;
    sh.depthWrite = true;
    sh.receiveShadows = false;
    sh.castShadows = false;
  }

  m_light = m_registry.createEntity("sun");
  m_registry.emplace<ecs::TransformComponent>(m_light);
  {
    auto& light = m_registry.emplace<ecs::LightComponent>(m_light);
    light.type = ecs::LightComponent::Type::Directional;
    // Placeholder values; SkyPresetSystem will drive these when linked from SkyComponent.
    light.direction = math::normalize(math::Vec3{-0.35f, -1.0f, -0.15f});
    light.intensity = 3.25f;
    light.color = {1.0f, 0.96f, 0.88f};
    light.castShadows = true;
  }

  // Sky (procedural clouds).
  ecs::EntityId skyEntity = ecs::kInvalidEntityId;
  {
    skyEntity = m_registry.createEntity("sky");
    m_registry.emplace<ecs::TransformComponent>(skyEntity);

    auto& skyc = m_registry.emplace<ecs::SkyComponent>(skyEntity);
    skyc.skyType = ecs::SkyComponent::SkyType::Night;
    skyc.useSkyTypePreset = true;
    skyc.linkedDirectionalLightEntity = m_light;
    skyc.cloudType = ecs::SkyComponent::CloudType::Scattered;
    skyc.quality = ecs::SkyComponent::Quality::High;
    skyc.cloudCoverage = 0.78f;
    skyc.cloudDensity = 0.65f;
    skyc.cloudScale = 1.0f;
    skyc.cloudSpeed = 0.020f;
    skyc.cloudWindDirection = {1.0f, 0.35f};
    skyc.cloudTimeScale = 1.0f;
    skyc.cloudTurbulence = 0.45f;
    skyc.cloudLightAbsorption = 0.55f;
    skyc.cloudHeightMeters = 220.0f;
    skyc.starsSeed = 4242u;
    skyc.starsIntensity = 2.1f;
    skyc.starsDensity = 0.70f;
    skyc.starsSize = 1.05f;
    skyc.starsTwinkleStrength = 0.22f;
    skyc.starsTwinkleSpeed = 0.55f;

    auto& sh = m_registry.emplace<ecs::ShaderComponent>(skyEntity, materials::presets::RealisticSkyClouds());
    sh.shader.key = "graphics/shaders/sky";
  }

  // Fog/mist volume (hide terrain edge).
  ecs::EntityId fogEntity = ecs::kInvalidEntityId;
  {
    fogEntity = m_registry.createEntity("mist");
    auto& tr = m_registry.emplace<ecs::TransformComponent>(fogEntity);
    tr.position = {0.0f, 6.0f, 0.0f};

    const float w = 96.0f * 1.0f;
    const float d = 96.0f * 1.0f;
    auto& f = m_registry.emplace<ecs::FogVolumeComponent>(fogEntity);
    f.sizeMeters = {w * 1.25f, 80.0f, d * 1.25f};
    // Placeholder values; SkyPresetSystem will drive these when linked from SkyComponent.
    // f.color = {0.55f, 0.62f, 0.72f, 1.0f};
    f.density = 5000000.0f;
    f.startDistance = 0.01f;
    f.endDistance = 160.0f;
    f.heightFalloff = 0.045f;
    f.baseHeightOffset = -4.0f;
    f.enabled = true;
  }

  if (skyEntity != ecs::kInvalidEntityId && fogEntity != ecs::kInvalidEntityId) {
    m_registry.get<ecs::SkyComponent>(skyEntity).linkedFogVolumeEntity = fogEntity;
  }

  // Apply once so the very first frame matches the chosen SkyType.
  m_skyPresets.tick(m_registry);

  // Force a snapshot on the first tick.
  m_printTimer = 0.5;

  // Global render switches (tweakable).
  {
    m_renderSettings = m_registry.createEntity("render_settings");
    auto& rs = m_registry.emplace<ecs::RenderSettingsComponent>(m_renderSettings);
    rs.enabled = true;
  }

  {
    m_playerHandSocket = m_registry.createEntity("player_hand_socket");
    m_registry.emplace<ecs::TransformComponent>(m_playerHandSocket);
    auto& socket = m_registry.emplace<ecs::SocketComponent>(m_playerHandSocket);
    socket.name = "player_hand_socket";
    socket.targetEntity = m_player;
    socket.targetEntityName = "business_man";
    socket.skeletonName = "business-man#skin0";
    socket.boneName = "RightHand_42";

    auto& combat = m_registry.emplace<ecs::CombatVolumeComponent>(m_playerHandSocket);
    ecs::CombatVolumeComponent::Volume hitVolume;
    hitVolume.role = ecs::CombatVolumeComponent::Role::Hit;
    hitVolume.shape = ecs::CombatVolumeComponent::Shape::Box;
    hitVolume.box = {0.30f, 0.22f, 0.45f};
    hitVolume.offset = {0.0f, 0.0f, 0.22f};
    hitVolume.damage = 12.0f;
    hitVolume.damageType = "melee";
    hitVolume.force = 4.5f;
    hitVolume.singleHit = true;
    combat.volumes.push_back(hitVolume);
    m_playerHitVolume = m_playerHandSocket;
  }

  {
    m_demoNpcHurtVolume = m_registry.createEntity("demo_npc_hurtbox");
    auto& tr = m_registry.emplace<ecs::TransformComponent>(m_demoNpcHurtVolume);
    tr.position = {0.0f, 0.0f, 0.0f};
    auto& attach = m_registry.emplace<ecs::AttachmentComponent>(m_demoNpcHurtVolume);
    ecs::AttachmentComponent::Attachment a;
    a.targetEntity = m_demoNpc;
    a.mode = ecs::AttachmentComponent::Mode::Parent;
    a.positionOffset = {0.0f, 0.0f, 0.0f};
    a.inheritRotation = true;
    a.inheritScale = false;
    attach.attachments.push_back(a);
    auto& combat = m_registry.emplace<ecs::CombatVolumeComponent>(m_demoNpcHurtVolume);
    ecs::CombatVolumeComponent::Volume hurtVolume;
    hurtVolume.role = ecs::CombatVolumeComponent::Role::Hurt;
    hurtVolume.shape = ecs::CombatVolumeComponent::Shape::Box;
    hurtVolume.box = {0.8f, 1.7f, 0.55f};
    hurtVolume.offset = {0.0f, 0.95f, 0.0f};
    hurtVolume.damageMultiplier = 1.0f;
    combat.volumes.push_back(hurtVolume);
  }

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  if (!m_renderer.start(1280, 720, "Duppy - Gameplay Demo")) {
    std::cerr << "OpenGL renderer failed to start; falling back to terminal snapshot.\n";
  }
#endif
}

void GameplayDemoGame::onTick(const core::TickContext& ctx) {
  for (const auto& line : ctx.inputLines) {
    if (line == "q" || line == "quit" || line == "exit") {
      if (ctx.requestQuit) ctx.requestQuit();
      return;
    }
  }

  // --- Input -> Controller requests ---
  m_events.clear();
  core::ControlService::RealtimeInput rt{};
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  if (m_renderer.isOpen()) {
    // Poll first so GLFW callbacks update key/mouse state before we drain it.
    m_renderer.pollEvents();
    const auto in = m_renderer.drainRealtimeInput();
    rt.hasInput = true;
    rt.moveX = in.moveX;
    rt.moveZ = in.moveZ;
    rt.sprint = in.sprint;
    rt.crouch = in.crouch;
    rt.jump = in.jump;
    rt.lookActive = in.lookActive;
    constexpr float kMouseToDeg = 0.08f;
    rt.lookDeltaYawDeg = in.mouseDx * kMouseToDeg;
    rt.lookDeltaPitchDeg = in.mouseDy * kMouseToDeg;
  }
#endif
  m_controls.update(ctx, rt.hasInput ? &rt : nullptr);
  m_controllerSystem.tick(m_registry, m_controls);

  if (auto* renderSettings = m_registry.tryGet<ecs::RenderSettingsComponent>(m_renderSettings)) {
    renderSettings->showRays = ctx.debugHudEnabled;
    renderSettings->showCollisionBoxes = ctx.debugHudEnabled;
    renderSettings->showCombatBoxes = ctx.debugHudEnabled;
    renderSettings->showSkeletonBones = ctx.debugHudEnabled;
  }

  if (const auto* controller = m_registry.tryGet<ecs::ControllerComponent>(m_player)) {
    if (hasActionRequest(controller, "attack")) {
      m_attackTimerSeconds = 0.25;
    }
  }
  if (m_attackTimerSeconds > 0.0) {
    m_attackTimerSeconds = std::max(0.0, m_attackTimerSeconds - ctx.deltaSeconds);
  }
  if (auto* combat = m_registry.tryGet<ecs::CombatVolumeComponent>(m_playerHitVolume)) {
    for (auto& volume : combat->volumes) {
      volume.enabled = m_attackTimerSeconds > 0.0;
      volume.singleHit = true;
    }
  }

  // --- Controller requests -> motion intent ---
  m_motionSystem.tick(m_registry);

  // --- Gravity only updates velocities ---
  m_gravitySystem.tick(m_registry, ctx.deltaSeconds);

  // --- Jump impulse request ---
  m_jumpSystem.tick(m_registry);

  // --- Motion intent -> transform movement ---
  m_movementSystem.tick(m_registry, m_events, ctx.deltaSeconds);

  // --- Collision detection emits contacts; resolution consumes them and separates bodies ---
  m_collisionDetectionSystem.tick(m_registry, m_events, ctx.elapsedSeconds);
  m_collisionResolutionSystem.tick(m_registry, m_events);

  // --- Skeleton assets -> animation -> IK -> sockets -> attachments -> rays -> sensors ---
  m_skeletonAssetSyncSystem.tick(m_registry, m_meshAssets);
  m_idleAnimationSystem.tick(m_registry, ctx.deltaSeconds);
  m_animationSystem.tick(m_registry, ctx.deltaSeconds);
  // m_ikSystem.tick(m_registry, m_events);
  m_hierarchySystem.tick(m_registry);
  m_attachmentSystem.update(m_registry, ctx.deltaSeconds);
  m_rayDetectionSystem.tick(m_registry, m_events, ctx.elapsedSeconds);
  m_sensorSystem.tick(m_registry, m_events);
  (void)updatePlayerVisionDrivenIk(m_registry, m_events, m_player, m_demoNpc);
  updatePlayerHeadFacingIk(m_registry, m_player, m_camera);
  m_ikSystem.tick(m_registry, m_events, ctx.deltaSeconds);
   m_socketSystem.tick(m_registry, ctx.deltaSeconds);
  m_hitDetectionSystem.tick(m_registry, m_events, ctx.elapsedSeconds);

  // --- Third-person camera follow ---
  m_thirdPersonCameraSystem.tick(m_registry, ctx.deltaSeconds);

  // --- Environment presets (SkyType -> sky/light/fog) ---
  m_skyPresets.tick(m_registry);

  const auto& frame = m_graphics.tick(m_registry);

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  if (m_renderer.isOpen()) {
    m_renderer.render(frame,
                      ctx.debugHudEnabled,
                      static_cast<float>(ctx.fpsEstimate),
                      static_cast<float>(ctx.deltaSeconds * 1000.0),
                      static_cast<float>(ctx.cpuWorkSeconds * 1000.0));
  } else {
    if (ctx.requestQuit) ctx.requestQuit();
    return;
  }
#endif

}

void GameplayDemoGame::onStop() {
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  m_renderer.stop();
#endif
  m_meshAssets.stop();
  std::cout << "\nStopped gameplay demo.\n";
}

}  // namespace games
