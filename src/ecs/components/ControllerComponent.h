#pragma once

// Author: Karl-Johan Bailey
//
// ControllerComponent (request interface)
// This component receives control requests from a control service (player input, AI, network, scripts).
// It is intentionally descriptive and "request-only" — movement/aim/shoot systems interpret requests.

#include "math/Vec3.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ecs {

struct ControllerComponent {
  enum class Mode {
    Player,
    AI,
    Network,
    Script,
  };

  enum class MoveMode {
    Walk,
    Sprint,
    Crouch,
  };

  bool enabled = true;
  Mode mode = Mode::Player;

  // Move request written by control service each frame (or when changed).
  struct MoveRequest {
    bool hasRequest = false;

    // Two common patterns:
    // 1) Direction-based (player)
    bool hasDirection = false;
    math::Vec3 direction{0.0f, 0.0f, 0.0f};
    MoveMode moveMode = MoveMode::Walk;

    // 2) Destination-based (AI)
    bool hasDestination = false;
    math::Vec3 destination{0.0f, 0.0f, 0.0f};
    float acceptanceRadius = 0.5f;
  } moveRequest;

  // Look request (e.g., mouse/controller stick, aim target, etc).
  struct LookRequest {
    bool hasRequest = false;
    math::Vec3 lookDelta{0.0f, 0.0f, 0.0f};   // pitch/yaw/roll delta (degrees; engine-defined)
    math::Vec3 lookDirection{0.0f, 0.0f, 1.0f}; // optional absolute direction
  } lookRequest;

  // Generic action requests (jump, attack, interact, etc).
  struct ActionRequest {
    std::string action;
    bool pressed = false;
    float value = 0.0f;
  };
  std::vector<ActionRequest> actionRequests;

  // Priority for resolving multiple controllers (higher wins).
  int priority = 0;
};

}  // namespace ecs

