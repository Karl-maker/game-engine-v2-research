#pragma once

// Author: Karl-Johan Bailey
//
// AudioSystem
// Placeholder audio playback tracker. Manages per-entity play state for AudioComponent.
// A real audio backend can be swapped in later without changing gameplay code.

#include "ecs/EntityRegistry.h"

#include <string>
#include <unordered_map>

namespace ecs::systems {

class AudioSystem final {
 public:
  void tick(EntityRegistry& registry, double deltaSeconds);

 private:
  struct VoiceState final {
    std::string clipKey;
    bool playing = false;
    bool loop = false;
    float volume = 1.0f;
    float pitch = 1.0f;
    float durationSeconds = 0.0f;
    float playheadSeconds = 0.0f;
    bool seen = false;
    bool startedOnce = false;
  };

  std::unordered_map<EntityId, VoiceState> m_voices;
};

}  // namespace ecs::systems
