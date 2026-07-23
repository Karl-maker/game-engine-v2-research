#include "ecs/services/IKService.h"

// Author: Karl-Johan Bailey

#include <algorithm>

namespace ecs::services {

ecs::IKComponent::Chain& IKService::ensureChain(ecs::IKComponent& ik,
                                                const std::string& chainName,
                                                const std::vector<std::string>& boneNames) {
  for (auto& chain : ik.chains) {
    if (chain.name == chainName) {
      if (!boneNames.empty()) chain.boneNames = boneNames;
      return chain;
    }
  }

  ecs::IKComponent::Chain chain;
  chain.name = chainName;
  chain.boneNames = boneNames;
  ik.chains.push_back(std::move(chain));
  return ik.chains.back();
}

void IKService::setEntityTarget(ecs::IKComponent::Chain& chain, ecs::EntityId targetEntity, const math::Vec3& offset) {
  chain.targetMode = ecs::IKComponent::Chain::TargetMode::Entity;
  chain.targetEntity = targetEntity;
  chain.targetEntityName.clear();
  chain.targetOffset = offset;
}

void IKService::setWorldTarget(ecs::IKComponent::Chain& chain, const math::Vec3& worldTarget) {
  chain.targetMode = ecs::IKComponent::Chain::TargetMode::WorldPosition;
  chain.targetEntity = ecs::kInvalidEntityId;
  chain.targetEntityName.clear();
  chain.worldTarget = worldTarget;
}

void IKService::setWeight(ecs::IKComponent::Chain& chain, float weight) {
  chain.weight = std::clamp(weight, 0.0f, 1.0f);
}

void IKService::setBlendTimes(ecs::IKComponent::Chain& chain, float blendInSeconds, float blendOutSeconds) {
  chain.blendInSeconds = std::max(0.0f, blendInSeconds);
  chain.blendOutSeconds = blendOutSeconds < 0.0f ? chain.blendInSeconds : std::max(0.0f, blendOutSeconds);
}

void IKService::setIterations(ecs::IKComponent::Chain& chain, int iterations) {
  chain.iterations = std::max(1, iterations);
}

}  // namespace ecs::services
