#include "ecs/systems/HudSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/HudComponent.h"
#include "ecs/components/StatsComponent.h"

#include <algorithm>

namespace ecs::systems {

void HudSystem::tick(EntityRegistry& registry) {
  registry.view<ecs::HudComponent>([&](ecs::EntityId, ecs::HudComponent& hud) {
    if (!hud.enabled) return;
    for (auto& widget : hud.widgets) {
      if (!widget.enabled) continue;
      if (widget.valueSource == ecs::HudComponent::ValueSource::StatsHealth && widget.sourceEntity != ecs::kInvalidEntityId) {
        if (const auto* stats = registry.tryGet<ecs::StatsComponent>(widget.sourceEntity)) {
          widget.value = std::max(0.0f, stats->health);
          widget.maxValue = std::max(0.0f, stats->maxHealth > 0.0f ? stats->maxHealth : stats->health);
        }
      }
    }
  });
}

}  // namespace ecs::systems

