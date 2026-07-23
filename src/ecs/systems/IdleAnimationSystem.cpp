#include "ecs/systems/IdleAnimationSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/AnimationComponent.h"
#include "ecs/components/CharacterComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/SkeletonComponent.h"

#include <algorithm>

namespace {

bool hasClip(const ecs::AnimationComponent& anim, const std::string& clip) {
  return std::find(anim.availableClips.begin(), anim.availableClips.end(), clip) != anim.availableClips.end();
}

}  // namespace

namespace ecs::systems {

void IdleAnimationSystem::tick(EntityRegistry& registry, double deltaSeconds) const {
  const float dt = static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.25));

  registry.view<ecs::AnimationComponent, ecs::MotionComponent, ecs::CharacterComponent>(
      [&](ecs::EntityId id, ecs::AnimationComponent& anim, const ecs::MotionComponent& motion, const ecs::CharacterComponent& character) {
        if (!anim.enabled || !character.enabled) return;

        if (motion.isMoving) {
          anim.idleElapsedSeconds = 0.0f;
          return;
        }

        anim.idleElapsedSeconds += dt;
        if (anim.idleElapsedSeconds < anim.idleDelaySeconds) return;

        if (anim.layers.empty()) {
          anim.layers.push_back({anim.idleAnimationLayer.empty() ? "Base Layer" : anim.idleAnimationLayer,
                                 1.0f,
                                 ecs::AnimationComponent::BlendMode::Override,
                                 {},
                                 anim.idleAnimationClip,
                                 "",
                                 0.0f});
          return;
        }

        auto& base = anim.layers.front();
        std::string desired = hasClip(anim, anim.idleAnimationClip) ? anim.idleAnimationClip : "Idle";
        if (const auto* skeleton = registry.tryGet<ecs::SkeletonComponent>(id)) {
          if (!skeleton->animationClips.empty()) {
            auto hasSkeletonClip = [&](const std::string& name) {
              for (const auto& clip : skeleton->animationClips) {
                if (clip.name == name) return true;
              }
              return false;
            };
            if (!hasSkeletonClip(desired)) desired = skeleton->animationClips.front().name;
          }
        }
        if (base.currentState == desired) return;

        base.nextState = desired;
        base.transition = 0.2f;
      });
}

}  // namespace ecs::systems
