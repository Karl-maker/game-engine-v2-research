#include "games/GameplayDemoGame.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"

#include "ecs/components/CameraComponent.h"
#include "ecs/components/ControllerComponent.h"
#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"
#include "materials/presets/HighQualityDirtRockLayer.h"
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
  camTr.position = {10.0f, 1.7f, -26.0f};
  camTr.rotation = {10.0f, 343.0f, 0.0f};  // pitch/yaw/roll (deg)
  m_registry.emplace<ecs::CameraComponent>(m_camera);
  m_registry.emplace<ecs::ControllerComponent>(m_camera);
  {
    auto& motion = m_registry.emplace<ecs::MotionComponent>(m_camera);
    motion.mode = ecs::MotionComponent::Mode::Flying;
    motion.isGrounded = false;
  }
  {
    auto& stats = m_registry.emplace<ecs::StatsComponent>(m_camera);
    stats.walkingSpeed = 6.0f;
    stats.runningSpeed = 10.0f;
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
    terrain.noise.frequency = 0.010f;
    terrain.noise.octaves = 2;
    terrain.noise.persistence = 0.45f;
    terrain.noise.lacunarity = 2.0f;

    auto& shader = m_registry.emplace<ecs::ShaderComponent>(m_terrain, materials::presets::HighQualityDirtRockLayer());
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
    // Moonlight.
    light.direction = math::normalize(math::Vec3{-0.25f, -1.0f, -0.35f});
    light.intensity = 1.45f;
    light.color = {0.78f, 0.86f, 1.0f};
    light.castShadows = true;
  }

  // Sky (procedural clouds).
  {
    const auto sky = m_registry.createEntity("sky");
    m_registry.emplace<ecs::TransformComponent>(sky);

    auto& skyc = m_registry.emplace<ecs::SkyComponent>(sky);
    skyc.skyType = ecs::SkyComponent::SkyType::Night;
    skyc.cloudType = ecs::SkyComponent::CloudType::Scattered;
    skyc.quality = ecs::SkyComponent::Quality::High;
    skyc.cloudCoverage = 0.38f;
    skyc.cloudDensity = 0.65f;
    skyc.cloudScale = 1.0f;
    skyc.cloudSpeed = 0.020f;
    skyc.cloudWindDirection = {1.0f, 0.35f};
    skyc.cloudTimeScale = 1.0f;
    skyc.cloudTurbulence = 0.45f;
    skyc.cloudLightAbsorption = 0.55f;
    skyc.cloudHeightMeters = 220.0f;
    // Use sun disc visuals as a moon at night.
    skyc.sunDirection = math::normalize(math::Vec3{-0.15f, 0.9f, -0.15f});
    skyc.sunTint = {0.72f, 0.80f, 1.0f, 1.0f};
    skyc.sunDiscIntensity = 0.75f;
    skyc.sunDiscSize = 0.65f;
    skyc.horizonColor = {0.04f, 0.06f, 0.13f, 1.0f};
    skyc.zenithColor = {0.015f, 0.03f, 0.09f, 1.0f};

    skyc.starsEnabled = true;
    skyc.starsSeed = 4242u;
    skyc.starsIntensity = 2.1f;
    skyc.starsDensity = 0.70f;
    skyc.starsSize = 1.05f;
    skyc.starsTwinkleStrength = 0.22f;
    skyc.starsTwinkleSpeed = 0.55f;

    auto& sh = m_registry.emplace<ecs::ShaderComponent>(sky, materials::presets::RealisticSkyClouds());
    sh.shader.key = "graphics/shaders/sky";
  }

  // Fog/mist volume (hide terrain edge).
  {
    const auto fog = m_registry.createEntity("mist");
    auto& tr = m_registry.emplace<ecs::TransformComponent>(fog);
    tr.position = {0.0f, 6.0f, 0.0f};

    const float w = 96.0f * 1.0f;
    const float d = 96.0f * 1.0f;
    auto& f = m_registry.emplace<ecs::FogVolumeComponent>(fog);
    f.sizeMeters = {w * 1.25f, 80.0f, d * 1.25f};
    f.color = {0.05f, 0.07f, 0.12f, 1.0f};
    // Thicker mist to hide terrain edge.
    f.density = 0.070f;
    f.startDistance = 4.0f;
    f.endDistance = 140.0f;
    f.heightFalloff = 0.030f;
    f.baseHeightOffset = -4.0f;
  }

  // Force a snapshot on the first tick.
  m_printTimer = 0.5;

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

  // --- Motion intent -> transform movement ---
  m_movementSystem.tick(m_registry, m_events, ctx.deltaSeconds);

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

    std::cout << "camera=(" << frame.camera.position.x << "," << frame.camera.position.y << "," << frame.camera.position.z
              << ") forward=(" << frame.camera.forward.x << "," << frame.camera.forward.y << "," << frame.camera.forward.z
              << ") terrains=" << frame.terrains.size() << " lights=" << frame.lights.size();

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
