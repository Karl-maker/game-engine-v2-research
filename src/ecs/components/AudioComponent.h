#pragma once

// Author: Karl-Johan Bailey
//
// AudioComponent (data + play requests)
// Describes a single audio clip attached to an entity. AudioSystem interprets this.

#include <string>

namespace ecs {

struct AudioComponent final {
  bool enabled = true;

  // Asset key/path to the audio clip (e.g. "assets/audio/footstep.wav").
  std::string clipKey;

  // Playback controls.
  bool loop = false;
  bool playOnStart = false;

  // Requests (set by gameplay). AudioSystem clears them after consuming.
  bool requestPlay = false;
  bool requestStop = false;
  bool restartIfPlaying = true;

  // Simple mix controls (backend-defined).
  float volume = 1.0f;  // 0..1
  float pitch = 1.0f;   // 1.0 = normal

  // Optional metadata for deterministic looping in the placeholder backend.
  // If 0, AudioSystem will treat the clip length as unknown (no auto-stop).
  float durationSeconds = 0.0f;

  // Runtime state (written by AudioSystem).
  bool playing = false;
  float playheadSeconds = 0.0f;
};

}  // namespace ecs

