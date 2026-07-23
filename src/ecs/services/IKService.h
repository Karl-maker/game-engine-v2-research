#pragma once

// Author: Karl-Johan Bailey
//
// IKService
// Small gameplay-facing helper for configuring IKComponent chains without
// requiring callers to manually edit the component layout.

#include "ecs/components/IKComponent.h"

#include <string>
#include <vector>

namespace ecs::services {

class IKService final {
 public:
  static ecs::IKComponent::Chain& ensureChain(ecs::IKComponent& ik,
                                              const std::string& chainName,
                                              const std::vector<std::string>& boneNames);

  static void setEntityTarget(ecs::IKComponent::Chain& chain, ecs::EntityId targetEntity, const math::Vec3& offset = {});
  static void setWorldTarget(ecs::IKComponent::Chain& chain, const math::Vec3& worldTarget);
  static void setWeight(ecs::IKComponent::Chain& chain, float weight);
  static void setIterations(ecs::IKComponent::Chain& chain, int iterations);
};

}  // namespace ecs::services
