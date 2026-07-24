#include "ecs/systems/AudioSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/AudioComponent.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace ecs::systems {

namespace {

static float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

}  // namespace

void AudioSystem::tick(EntityRegistry& registry, double deltaSeconds) {
  const float dt = static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.25));

  // Mark all as unseen, then mark seen during view, and finally erase unseen.
  for (auto& kv : m_voices) kv.second.seen = false;

  registry.view<ecs::AudioComponent>([&](ecs::EntityId id, ecs::AudioComponent& audio) {
    VoiceState& st = m_voices[id];
    st.seen = true;

    if (!audio.enabled || audio.clipKey.empty()) {
      st.playing = false;
      st.playheadSeconds = 0.0f;
      audio.playing = false;
      audio.playheadSeconds = 0.0f;
      audio.requestPlay = false;
      audio.requestStop = false;
      return;
    }

    // Sync configuration.
    st.loop = audio.loop;
    st.volume = clamp01(audio.volume);
    st.pitch = std::max(0.01f, audio.pitch);
    st.durationSeconds = std::max(0.0f, audio.durationSeconds);

    // If the clip changed, restart.
    if (st.clipKey != audio.clipKey) {
      st.clipKey = audio.clipKey;
      st.playing = false;
      st.playheadSeconds = 0.0f;
    }

    // Autoplay once.
    if (audio.playOnStart && !st.startedOnce) {
      audio.requestPlay = true;
    }

    if (audio.requestStop) {
      st.playing = false;
      st.playheadSeconds = 0.0f;
    }

    if (audio.requestPlay) {
      if (!st.playing || audio.restartIfPlaying) {
        st.playheadSeconds = 0.0f;
      }
      st.playing = true;
      st.startedOnce = true;
    }

    // Advance playhead (placeholder backend).
    if (st.playing) {
      st.playheadSeconds += dt * st.pitch;
      if (st.durationSeconds > 0.0f) {
        if (st.playheadSeconds >= st.durationSeconds) {
          if (st.loop) {
            st.playheadSeconds = std::fmod(st.playheadSeconds, st.durationSeconds);
          } else {
            st.playing = false;
            st.playheadSeconds = st.durationSeconds;
          }
        }
      }
    }

    // Write runtime state back to the component.
    audio.playing = st.playing;
    audio.playheadSeconds = st.playheadSeconds;

    // Consume requests.
    audio.requestPlay = false;
    audio.requestStop = false;
  });

  // Reclaim voices for entities that no longer have AudioComponent.
  for (auto it = m_voices.begin(); it != m_voices.end();) {
    if (!it->second.seen) it = m_voices.erase(it);
    else ++it;
  }
}

}  // namespace ecs::systems

