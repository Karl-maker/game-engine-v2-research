#include "games/GameplayDemoGame.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"

#include "ecs/components/CameraComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"
#include "materials/presets/PebblyDirt.h"

#include <iostream>

namespace games {

void GameplayDemoGame::onStart() {
  std::cout << "Gameplay demo (graphics snapshot)\n";
  std::cout << "- Creates camera + terrain(shader) + light\n";
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  std::cout << "- Opens an OpenGL window and renders the terrain\n";
#else
  std::cout << "- Runs GraphicsSystem each tick (prints a snapshot)\n";
#endif
  std::cout << "Type `q` then Enter to quit.\n\n";

  m_camera = m_registry.createEntity("camera");
  auto& camTr = m_registry.emplace<ecs::TransformComponent>(m_camera);
  camTr.position = {10.0f, 14.0f, -26.0f};
  camTr.rotation = {28.0f, 335.0f, 0.0f};  // pitch/yaw/roll (deg)
  m_registry.emplace<ecs::CameraComponent>(m_camera);

  m_terrain = m_registry.createEntity("terrain");
  m_registry.emplace<ecs::TransformComponent>(m_terrain);
  {
    auto& terrain = m_registry.emplace<ecs::TerrainComponent>(m_terrain);
    terrain.gridWidth = 96;
    terrain.gridHeight = 96;
    terrain.cellSizeMeters = 1.0f;
    terrain.heightScaleMeters = 35.0f;

    auto& shader = m_registry.emplace<ecs::ShaderComponent>(m_terrain, materials::presets::PebblyDirt());
    // Render using the current OpenGL demo shader (textures are ignored for now).
    shader.shader.key = "graphics/shaders/terrain";
    shader.depthWrite = true;
  }

  m_light = m_registry.createEntity("sun");
  m_registry.emplace<ecs::TransformComponent>(m_light);
  {
    auto& light = m_registry.emplace<ecs::LightComponent>(m_light);
    light.type = ecs::LightComponent::Type::Directional;
    light.direction = math::normalize(math::Vec3{-0.2f, -1.0f, -0.3f});
    light.intensity = 3.0f;
    light.color = {1.0f, 0.98f, 0.92f};
    light.castShadows = true;
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

  // Slowly orbit the camera yaw so the facing direction changes over time.
  if (auto* tr = m_registry.tryGet<ecs::TransformComponent>(m_camera)) {
    tr->rotation.y += static_cast<float>(15.0 * ctx.deltaSeconds);
    if (tr->rotation.y > 360.0f) tr->rotation.y -= 360.0f;
  }

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
