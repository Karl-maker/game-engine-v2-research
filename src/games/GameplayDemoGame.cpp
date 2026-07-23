#include "games/GameplayDemoGame.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"

#include "ecs/components/CameraComponent.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/CharacterComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/ControllerComponent.h"
#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/RenderSettingsComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/GrassPatchComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/ThirdPersonCameraComponent.h"
#include "ecs/components/TransformComponent.h"
#include "materials/presets/HighQualityDirtRockLayer.h"
#include "materials/presets/StoneGrass.h"
#include "materials/presets/HighQualityDirtRockGrassLayer.h"
#include "materials/presets/RealisticSkyClouds.h"

#include <iostream>

namespace games {

void GameplayDemoGame::onStart() {
  std::cout << "Gameplay demo (graphics snapshot)\n";
  std::cout << "- Creates camera + terrain(shader) + light\n";
  std::cout << "- Camera is driven by Controller/Motion/Movement systems\n";
  std::cout << "Controls (type then Enter): w/a/s/d, move x y z, look pitch yaw, sprint/walk/crouch, stop\n";
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  std::cout << "- Opens an OpenGL window and renders the terrain\n";
#else
  std::cout << "- Runs GraphicsSystem each tick (prints a snapshot)\n";
#endif
  std::cout << "Type `q` then Enter to quit.\n\n";

  m_camera = m_registry.createEntity("camera");
  auto& camTr = m_registry.emplace<ecs::TransformComponent>(m_camera);
  camTr.position = {0.0f, 3.0f, -6.0f};
  camTr.rotation = {12.0f, 0.0f, 0.0f};  // pitch/yaw/roll (deg)
  m_registry.emplace<ecs::CameraComponent>(m_camera);

  m_player = m_registry.createEntity("business_man");
  auto& playerTr = m_registry.emplace<ecs::TransformComponent>(m_player);
  playerTr.position = {0.0f, 0.0f, -8.0f};
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
  }
  {
    auto& stats = m_registry.emplace<ecs::StatsComponent>(m_player);
    stats.walkingSpeed = 3.8f;
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
  }
  {
    auto& thirdPerson = m_registry.emplace<ecs::ThirdPersonCameraComponent>(m_camera);
    thirdPerson.target = m_player;
    thirdPerson.targetOffset = {0.0f, 1.25f, 0.0f};
    thirdPerson.distance = 4.2f;
    thirdPerson.height = 0.8f;
    thirdPerson.yawDeg = playerTr.rotation.y;
  }

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
    collider.terrain.enabled = true;
    collider.terrain.sourceTerrainEntity = m_terrain;
    collider.terrain.thicknessMeters = 5.0f;

    auto& shader = m_registry.emplace<ecs::ShaderComponent>(m_terrain, materials::presets::HighQualityDirtRockGrassLayer());
    // Render using the current OpenGL demo shader (textures are ignored for now).
    shader.shader.key = "graphics/shaders/terrain";
    shader.depthWrite = true;

    // Scene override: remove the circular "sink" patches (often mistaken for pebbles).
    shader.parameters.push_back({"dirtSinksEnabled", false});
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
    f.color = {0.55f, 0.62f, 0.72f, 1.0f};
    f.density = 0.0f;
    f.startDistance = 6.0f;
    f.endDistance = 160.0f;
    f.heightFalloff = 0.045f;
    f.baseHeightOffset = -4.0f;
    f.enabled = false;
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
    const auto rs = m_registry.createEntity("render_settings");
    (void)m_registry.emplace<ecs::RenderSettingsComponent>(rs);
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
    rt.lookActive = in.lookActive;
    constexpr float kMouseToDeg = 0.08f;
    rt.lookDeltaYawDeg = in.mouseDx * kMouseToDeg;
    rt.lookDeltaPitchDeg = in.mouseDy * kMouseToDeg;
  }
#endif
  m_controls.update(ctx, rt.hasInput ? &rt : nullptr);
  m_controllerSystem.tick(m_registry, m_controls);

  // --- Controller requests -> motion intent ---
  m_motionSystem.tick(m_registry);

  // --- Gravity only updates velocities ---
  m_gravitySystem.tick(m_registry, ctx.deltaSeconds);

  // --- Motion intent -> transform movement ---
  m_movementSystem.tick(m_registry, m_events, ctx.deltaSeconds);

  // --- Collision detection emits contacts; resolution consumes them and separates bodies ---
  m_collisionDetectionSystem.tick(m_registry, m_events, ctx.elapsedSeconds);
  m_collisionResolutionSystem.tick(m_registry, m_events);

  // --- Animation + third-person camera follow ---
  m_animationSystem.tick(m_registry, ctx.deltaSeconds);
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

  m_printTimer += ctx.deltaSeconds;
  if (m_printTimer >= 0.5) {
    m_printTimer = 0.0;

    const auto* playerTr = m_registry.tryGet<ecs::TransformComponent>(m_player);
    std::cout << "camera=(" << frame.camera.position.x << "," << frame.camera.position.y << "," << frame.camera.position.z
              << ") forward=(" << frame.camera.forward.x << "," << frame.camera.forward.y << "," << frame.camera.forward.z
              << ") player=(" << (playerTr ? playerTr->position.x : 0.0f) << "," << (playerTr ? playerTr->position.y : 0.0f)
              << "," << (playerTr ? playerTr->position.z : 0.0f) << ") meshes=" << frame.meshes.size()
              << " terrains=" << frame.terrains.size() << " lights=" << frame.lights.size();

    if (!frame.terrains.empty()) {
      std::cout << " terrain.shader=\"" << frame.terrains[0].shader.key << "\"";
    }
    std::cout << "\n";
  }
}

void GameplayDemoGame::onStop() {
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  m_renderer.stop();
#endif
  std::cout << "\nStopped gameplay demo.\n";
}

}  // namespace games
