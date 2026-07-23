#include "ecs/systems/AnimationSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/AnimationComponent.h"
#include "ecs/components/MotionComponent.h"

#include <algorithm>

namespace ecs::systems {

void AnimationSystem::tick(EntityRegistry& registry, double deltaSeconds) const {
  const float dt = static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.25));
  registry.view<ecs::AnimationComponent>([&](ecs::EntityId id, ecs::AnimationComponent& anim) {
    if (!anim.enabled) return;
    if (anim.layers.empty()) {
      anim.layers.push_back({"Base Layer", 1.0f, ecs::AnimationComponent::BlendMode::Override, {}, "Idle", "", 0.0f});
    }

    const auto* motion = registry.tryGet<ecs::MotionComponent>(id);
    std::string desired = "Idle";
    if (motion && motion->isMoving) {
      desired = (motion->currentSpeed > 6.5f) ? "Run" : "Walk";
    }

    auto hasClip = [&](const std::string& name) {
      return std::find(anim.availableClips.begin(), anim.availableClips.end(), name) != anim.availableClips.end();
    };
    if (!hasClip(desired) && !anim.availableClips.empty()) {
      desired = anim.availableClips.front();
    }

    auto& base = anim.layers.front();
    if (base.currentState != desired) {
      base.nextState = desired;
      base.transition = 0.12f;
    }
    if (base.transition > 0.0f) {
      base.transition = std::max(0.0f, base.transition - dt);
      if (base.transition == 0.0f) {
        base.currentState = base.nextState;
        base.nextState.clear();
      }
    } else {
      base.currentState = desired;
    }

    anim.currentFrame += dt * 30.0f;
  });
}

}  // namespace ecs::systems
