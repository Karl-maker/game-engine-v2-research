#include "ecs/systems/AnimationSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/AnimationComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/SkeletonComponent.h"

#include <algorithm>
#include <cmath>

namespace ecs::systems {

namespace {

struct LocalTransform final {
  math::Vec3 translation{0.0f, 0.0f, 0.0f};
  math::Quat rotation{};
  math::Vec3 scale{1.0f, 1.0f, 1.0f};
  bool hasTranslation = false;
  bool hasRotation = false;
  bool hasScale = false;
};

math::Vec3 translationFromMat4(const math::Mat4& m) {
  return {m.m[12], m.m[13], m.m[14]};
}

math::Vec3 scaleFromMat4(const math::Mat4& m) {
  const math::Vec3 x{m.m[0], m.m[1], m.m[2]};
  const math::Vec3 y{m.m[4], m.m[5], m.m[6]};
  const math::Vec3 z{m.m[8], m.m[9], m.m[10]};
  return {std::max(0.0001f, math::length(x)), std::max(0.0001f, math::length(y)), std::max(0.0001f, math::length(z))};
}

int sampleSegment(const std::vector<float>& times, float time, float& alpha) {
  alpha = 0.0f;
  if (times.size() < 2 || time <= times.front()) return 0;
  for (std::size_t i = 0; i + 1 < times.size(); ++i) {
    if (time <= times[i + 1]) {
      const float span = std::max(0.000001f, times[i + 1] - times[i]);
      alpha = std::clamp((time - times[i]) / span, 0.0f, 1.0f);
      return static_cast<int>(i);
    }
  }
  return static_cast<int>(times.size() - 2);
}

math::Vec3 sampleVec3(const std::vector<float>& times, const std::vector<math::Vec3>& values, float time) {
  if (values.empty()) return {};
  if (values.size() == 1 || times.size() < 2) return values.front();
  float alpha = 0.0f;
  const int idx = sampleSegment(times, time, alpha);
  const std::size_t a = static_cast<std::size_t>(std::clamp(idx, 0, static_cast<int>(values.size() - 1)));
  const std::size_t b = std::min(a + 1, values.size() - 1);
  return math::lerp(values[a], values[b], alpha);
}

math::Quat sampleQuat(const std::vector<float>& times, const std::vector<math::Quat>& values, float time) {
  if (values.empty()) return {};
  if (values.size() == 1 || times.size() < 2) return values.front();
  float alpha = 0.0f;
  const int idx = sampleSegment(times, time, alpha);
  const std::size_t a = static_cast<std::size_t>(std::clamp(idx, 0, static_cast<int>(values.size() - 1)));
  const std::size_t b = std::min(a + 1, values.size() - 1);
  return math::slerp(values[a], values[b], alpha);
}

const ecs::SkeletonComponent::AnimationClip* findClip(const ecs::SkeletonComponent& skeleton, const std::string& name) {
  for (const auto& clip : skeleton.animationClips) {
    if (clip.name == name) return &clip;
  }
  return skeleton.animationClips.empty() ? nullptr : &skeleton.animationClips.front();
}

void applyClipToSkeleton(ecs::SkeletonComponent& skeleton, const std::string& clipName, float currentFrame) {
  if (skeleton.bindPose.size() != skeleton.bones.size()) return;
  skeleton.currentPose = skeleton.bindPose;

  const auto* clip = findClip(skeleton, clipName);
  if (!clip || clip->channels.empty()) return;

  std::vector<LocalTransform> locals(skeleton.bones.size());
  for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
    locals[i].translation = translationFromMat4(skeleton.bindPose[i]);
    locals[i].scale = scaleFromMat4(skeleton.bindPose[i]);
  }

  const float duration = std::max(0.0001f, clip->durationSeconds);
  const float seconds = std::fmod(currentFrame / 30.0f, duration);

  for (const auto& channel : clip->channels) {
    if (channel.boneIndex < 0 || static_cast<std::size_t>(channel.boneIndex) >= locals.size()) continue;
    auto& local = locals[static_cast<std::size_t>(channel.boneIndex)];
    switch (channel.path) {
      case ecs::SkeletonComponent::AnimationClip::Path::Rotation:
        local.rotation = sampleQuat(channel.times, channel.quatValues, seconds);
        local.hasRotation = true;
        break;
      case ecs::SkeletonComponent::AnimationClip::Path::Scale:
        local.scale = sampleVec3(channel.times, channel.vec3Values, seconds);
        local.hasScale = true;
        break;
      case ecs::SkeletonComponent::AnimationClip::Path::Translation:
      default:
        local.translation = sampleVec3(channel.times, channel.vec3Values, seconds);
        local.hasTranslation = true;
        break;
    }
  }

  for (std::size_t i = 0; i < locals.size(); ++i) {
    if (locals[i].hasTranslation || locals[i].hasRotation || locals[i].hasScale) {
      skeleton.currentPose[i] = math::compose(locals[i].translation, locals[i].rotation, locals[i].scale);
    }
  }
}

}  // namespace

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

    const auto* skeleton = registry.tryGet<ecs::SkeletonComponent>(id);
    auto hasClip = [&](const std::string& name) {
      if (skeleton && !skeleton->animationClips.empty()) {
        for (const auto& clip : skeleton->animationClips) {
          if (clip.name == name) return true;
        }
        return false;
      }
      return std::find(anim.availableClips.begin(), anim.availableClips.end(), name) != anim.availableClips.end();
    };
    if (!hasClip(desired) && skeleton && !skeleton->animationClips.empty()) {
      desired = skeleton->animationClips.front().name;
    } else if (!hasClip(desired) && !anim.availableClips.empty()) {
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
    if (skeleton) {
      auto* mutableSkeleton = registry.tryGet<ecs::SkeletonComponent>(id);
      if (mutableSkeleton) {
        applyClipToSkeleton(*mutableSkeleton, base.currentState.empty() ? desired : base.currentState, anim.currentFrame);
      }
    }
  });
}

}  // namespace ecs::systems
