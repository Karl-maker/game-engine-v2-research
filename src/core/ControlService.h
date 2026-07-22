#pragma once

// Author: Karl-Johan Bailey
//
// ControlService (demo-focused)
// Converts input (currently line commands from TickContext) into a simple control state
// that ControllerSystem can apply to ControllerComponents.

#include "math/Vec3.h"

#include <string>

namespace core {

struct TickContext;

class ControlService final {
 public:
  struct RealtimeInput final {
    bool hasInput = false;
    float moveX = 0.0f;   // -1..1
    float moveZ = 0.0f;   // -1..1
    bool sprint = false;
    bool crouch = false;
    bool lookActive = false;
    float lookDeltaPitchDeg = 0.0f;
    float lookDeltaYawDeg = 0.0f;
  };

  struct ControlState final {
    math::Vec3 moveDirection{0.0f, 0.0f, 0.0f};  // persistent until changed
    math::Vec3 lookDeltaDeg{0.0f, 0.0f, 0.0f};   // one-frame impulse; cleared each update()
    bool sprint = false;
    bool crouch = false;
  };

  void update(const TickContext& ctx, const RealtimeInput* realtime = nullptr);

  const ControlState& state() const { return m_state; }

 private:
  static bool parseVec3(const std::string& s, math::Vec3& out);
  static bool parseTwoFloats(const std::string& s, float& a, float& b);

  ControlState m_state{};
};

}  // namespace core
