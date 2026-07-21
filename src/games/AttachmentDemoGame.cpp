#include "games/AttachmentDemoGame.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"
#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/systems/AttachmentSystem.h"

#include <iostream>

namespace games {

void AttachmentDemoGame::onStart() {
  std::cout << "Attachment demo\n";
  std::cout << "- player moves forward\n";
  std::cout << "- camera follows player (offset: 0, 1.8, -5)\n";
  std::cout << "Type `q` then Enter to quit.\n\n";

  m_player = m_registry.createEntity("player");
  m_registry.emplace<ecs::TransformComponent>(m_player);

  m_camera = m_registry.createEntity("camera");
  auto& camTr = m_registry.emplace<ecs::TransformComponent>(m_camera);
  camTr.position = {0.0f, 1.8f, -5.0f};

  auto& attach = m_registry.emplace<ecs::AttachmentComponent>(m_camera);
  ecs::AttachmentComponent::Attachment a;
  a.targetEntity = m_player;
  a.mode = ecs::AttachmentComponent::Mode::Follow;
  a.positionOffset = {0.0f, 1.8f, -5.0f};
  a.positionSpeed = 8.0f;
  a.rotationSpeed = 12.0f;
  a.enabled = true;
  attach.attachments.push_back(a);
}

void AttachmentDemoGame::onTick(const core::TickContext& ctx) {
  // --- Game logic ---
  // Move player forward using delta time.
  auto& playerTr = m_registry.get<ecs::TransformComponent>(m_player);
  playerTr.position.z += static_cast<float>(2.0 * ctx.deltaSeconds);

  // --- Systems ---
  // Attachment updates camera position based on player.
  ecs::systems::AttachmentSystem attachmentSystem;
  attachmentSystem.update(m_registry, ctx.deltaSeconds);

  // --- Print a snapshot to terminal ---
  m_printTimer += ctx.deltaSeconds;
  if (m_printTimer >= 0.25) {
    m_printTimer = 0.0;

    const auto& camTr = m_registry.get<ecs::TransformComponent>(m_camera);
    std::cout << "player.z=" << playerTr.position.z << " | camera=(" << camTr.position.x << "," << camTr.position.y
              << "," << camTr.position.z << ")\n";
  }
}

void AttachmentDemoGame::onStop() { std::cout << "\nStopped attachment demo.\n"; }

}  // namespace games
