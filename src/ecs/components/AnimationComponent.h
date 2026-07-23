#pragma once

// Author: Karl-Johan Bailey

#include <string>
#include <vector>

namespace ecs {

struct AnimationComponent final {
  enum class BlendMode {
    Override,
    Additive,
  };

  struct AnimationLayer final {
    std::string name = "Base Layer";
    float weight = 1.0f;
    BlendMode blendMode = BlendMode::Override;
    std::vector<std::string> mask;
    std::string currentState;
    std::string nextState;
    float transition = 0.0f;
  };

  bool enabled = true;
  float currentFrame = 0.0f;
  std::vector<std::string> availableClips;
  std::vector<AnimationLayer> layers;

  float idleElapsedSeconds = 0.0f;
  float idleDelaySeconds = 5.0f;
  std::string idleAnimationClip = "Idle";
  std::string idleAnimationLayer = "Base Layer";
};

}  // namespace ecs
