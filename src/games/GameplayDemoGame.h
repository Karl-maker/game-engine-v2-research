#pragma once

// Author: Karl-Johan Bailey
//
// Gameplay demo:
// - Spawns a camera, a terrain entity with a shader, and a light.
// - Runs GraphicsSystem each tick and prints a small render snapshot periodically.

#include "core/IGame.h"

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "ecs/systems/GraphicsSystem.h"

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
#include "graphics/OpenGlRenderer.h"
#endif

namespace games {

class GameplayDemoGame final : public core::IGame {
 public:
  void onStart() override;
  void onTick(const core::TickContext& ctx) override;
  void onStop() override;

 private:
  ecs::EntityRegistry m_registry;
  ecs::systems::GraphicsSystem m_graphics;

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  graphics::OpenGlRenderer m_renderer;
#endif

  ecs::EntityId m_camera = ecs::kInvalidEntityId;
  ecs::EntityId m_terrain = ecs::kInvalidEntityId;
  ecs::EntityId m_light = ecs::kInvalidEntityId;

  double m_printTimer = 0.0;
};

}  // namespace games
