#pragma once

// Author: Karl-Johan Bailey
//
// Angle helpers for simple yaw/pitch interpolation (degrees).

#include <cmath>

namespace ecs::systems {

static constexpr float kPi = 3.14159265358979323846f;

inline float radToDeg(float r) { return r * (180.0f / kPi); }
inline float degToRad(float d) { return d * (kPi / 180.0f); }

inline float wrapDegrees(float deg) {
  float d = std::fmod(deg, 360.0f);
  if (d < -180.0f) d += 360.0f;
  if (d > 180.0f) d -= 360.0f;
  return d;
}

inline float deltaAngleDegrees(float fromDeg, float toDeg) { return wrapDegrees(toDeg - fromDeg); }

inline float moveTowardsAngleDegrees(float currentDeg, float targetDeg, float maxDeltaDeg) {
  const float delta = deltaAngleDegrees(currentDeg, targetDeg);
  if (std::fabs(delta) <= maxDeltaDeg) return targetDeg;
  return currentDeg + (delta > 0.0f ? maxDeltaDeg : -maxDeltaDeg);
}

}  // namespace ecs::systems

