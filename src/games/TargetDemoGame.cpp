#include "games/TargetDemoGame.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/TargetComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/systems/TargetSystem.h"

#include <cmath>
#include <iostream>

namespace games {

void TargetDemoGame::onStart() {
  std::cout << "Target demo\n";
  std::cout << "- player moves in a circle\n";
  std::cout << "- camera stays in place and rotates to face player\n";
  std::cout << "Type `q` then Enter to quit.\n\n";

  m_player = m_registry.createEntity("player");
  auto& playerTr = m_registry.emplace<ecs::TransformComponent>(m_player);
  playerTr.position = {0.0f, 0.0f, 0.0f};

  m_camera = m_registry.createEntity("camera");
  auto& camTr = m_registry.emplace<ecs::TransformComponent>(m_camera);
  camTr.position = {0.0f, 2.0f, -8.0f};

  auto& target = m_registry.emplace<ecs::TargetComponent>(m_camera);
  target.targetEntity = m_player;
  target.targetOffset = {0.0f, 1.2f, 0.0f};
  target.rotationSpeed = 180.0f;
  target.lockRoll = true;
  target.enabled = true;
}

void TargetDemoGame::onTick(const core::TickContext& ctx) {
  m_time += ctx.deltaSeconds;

  // --- Game logic ---
  // Move player around a circle.
  auto& playerTr = m_registry.get<ecs::TransformComponent>(m_player);
  const float radius = 4.0f;
  playerTr.position.x = radius * std::cos(static_cast<float>(m_time));
  playerTr.position.z = radius * std::sin(static_cast<float>(m_time));

  // --- Systems ---
  ecs::systems::TargetSystem targetSystem;
  targetSystem.update(m_registry, ctx.deltaSeconds);

  // --- Print a snapshot to terminal ---
  m_printTimer += ctx.deltaSeconds;
  if (m_printTimer >= 0.25) {
    m_printTimer = 0.0;
    const auto& camTr = m_registry.get<ecs::TransformComponent>(m_camera);
    std::cout << "player=(" << playerTr.position.x << "," << playerTr.position.y << "," << playerTr.position.z
              << ") | camera rot(p,y,r)=(" << camTr.rotation.x << "," << camTr.rotation.y << "," << camTr.rotation.z
              << ")\n";
  }
}

void TargetDemoGame::onStop() { std::cout << "\nStopped target demo.\n"; }

}  // namespace games
