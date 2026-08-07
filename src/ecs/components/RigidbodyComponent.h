#pragma once

// Author: Karl-Johan Bailey
//
// RigidbodyComponent (descriptive only)
// Stores physics body properties. A physics system is responsible for integrating velocities
// and applying the result back to TransformComponent.

#include "math/Vec3.h"

namespace ecs {

struct RigidbodyComponent {
  float mass = 1.0f;
  float inverseMass = 1.0f;  // convenience/cache (set by physics system when mass changes)

  math::Vec3 linearVelocity{0.0f, 0.0f, 0.0f};
  math::Vec3 angularVelocity{0.0f, 0.0f, 0.0f};  // pitch/yaw/roll velocity (deg/s or rad/s engine-defined)

  float linearDamping = 0.0f;
  float angularDamping = 0.0f;

  bool useGravity = true;
  bool kinematic = false;

  bool freezePositionX = false;
  bool freezePositionY = false;
  bool freezePositionZ = false;

  bool freezeRotationX = false;
  bool freezeRotationY = false;
  bool freezeRotationZ = false;

  bool sleepEnabled = true;

  math::Vec3 centerOfMass{0.0f, 0.0f, 0.0f};

  bool buoyant = false;
  float buoyancyHeight = -0.5;
};

}  // namespace ecs

