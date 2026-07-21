#pragma once

// Author: Karl-Johan Bailey
//
// Target demo:
// - Creates `player` and `camera` entities.
// - Player moves in a circle.
// - Camera does NOT move, it only rotates to face player using TargetComponent.

#include "core/IGame.h"

#include "ecs/EntityRegistry.h"

namespace games {

class TargetDemoGame final : public core::IGame {
 public:
  void onStart() override;
  void onTick(const core::TickContext& ctx) override;
  void onStop() override;

 private:
  ecs::EntityRegistry m_registry;
  ecs::EntityId m_player = ecs::kInvalidEntityId;
  ecs::EntityId m_camera = ecs::kInvalidEntityId;
  double m_printTimer = 0.0;
  double m_time = 0.0;
};

}  // namespace games

