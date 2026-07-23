#include "ecs/systems/SkeletonAssetSyncSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/MeshComponent.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "math/Mat4.h"

#include <algorithm>

namespace ecs::systems {

void SkeletonAssetSyncSystem::tick(EntityRegistry& registry, assets::MeshAssetService& assetService) const {
  registry.view<ecs::MeshComponent, ecs::SkeletonComponent>([&](ecs::EntityId id,
                                                                 const ecs::MeshComponent& mesh,
                                                                 ecs::SkeletonComponent& skeleton) {
    if (!mesh.meshData.enabled || mesh.meshData.key.empty()) return;

    auto ready = assetService.takeReady(mesh.meshData.key);
    if (!ready) {
      (void)assetService.request(mesh.meshData.key);
      return;
    }

    const auto& src = *ready;
    if (src.skeleton.bones.empty()) return;
    if (skeleton.skeletonId == src.skeleton.id && skeleton.boneCount == static_cast<int>(src.skeleton.bones.size())) return;

    skeleton.enabled = true;
    skeleton.skeletonId = src.skeleton.id;
    skeleton.skeletonData = src.sourcePath;
    skeleton.rootBone = src.skeleton.rootBone;
    skeleton.boneCount = static_cast<int>(src.skeleton.bones.size());
    skeleton.bones.clear();
    skeleton.bones.reserve(src.skeleton.bones.size());
    skeleton.currentPose.clear();
    skeleton.bindPose.clear();
    skeleton.inverseBindMatrices.clear();
    skeleton.animationClips.clear();
    skeleton.currentPose.reserve(src.skeleton.bones.size());
    skeleton.bindPose.reserve(src.skeleton.bones.size());
    skeleton.inverseBindMatrices.reserve(src.skeleton.bones.size());

    for (std::size_t i = 0; i < src.skeleton.bones.size(); ++i) {
      const auto& bone = src.skeleton.bones[i];
      ecs::SkeletonComponent::Bone dst;
      dst.key = bone.name;
      dst.parentIndex = bone.parentIndex;
      dst.localBindTransform = bone.localBindTransform;
      skeleton.bones.push_back(dst);
      skeleton.currentPose.push_back(dst.localBindTransform);
      skeleton.bindPose.push_back(dst.localBindTransform);
      skeleton.inverseBindMatrices.push_back(bone.inverseBindMatrix);
    }

    skeleton.animationClips.reserve(src.animations.size());
    for (const auto& srcClip : src.animations) {
      ecs::SkeletonComponent::AnimationClip clip;
      clip.name = srcClip.name;
      clip.durationSeconds = srcClip.durationSeconds;
      clip.channels.reserve(srcClip.channels.size());
      for (const auto& srcChannel : srcClip.channels) {
        ecs::SkeletonComponent::AnimationClip::Channel channel;
        channel.boneIndex = srcChannel.boneIndex;
        channel.times = srcChannel.times;
        channel.vec3Values = srcChannel.vec3Values;
        channel.quatValues = srcChannel.quatValues;
        switch (srcChannel.path) {
          case assets::LoadedAnimation::Path::Rotation:
            channel.path = ecs::SkeletonComponent::AnimationClip::Path::Rotation;
            break;
          case assets::LoadedAnimation::Path::Scale:
            channel.path = ecs::SkeletonComponent::AnimationClip::Path::Scale;
            break;
          case assets::LoadedAnimation::Path::Translation:
          default:
            channel.path = ecs::SkeletonComponent::AnimationClip::Path::Translation;
            break;
        }
        clip.channels.push_back(std::move(channel));
      }
      skeleton.animationClips.push_back(std::move(clip));
    }

    if (auto* anim = registry.tryGet<ecs::AnimationComponent>(id)) {
      for (const auto& clip : skeleton.animationClips) {
        const bool has = std::find(anim->availableClips.begin(), anim->availableClips.end(), clip.name) != anim->availableClips.end();
        if (!has) anim->availableClips.push_back(clip.name);
      }
      if (!skeleton.animationClips.empty() && anim->idleAnimationClip.empty()) {
        anim->idleAnimationClip = skeleton.animationClips.front().name;
      }
    }
  });
}

}  // namespace ecs::systems
