#pragma once

// Author: Karl-Johan Bailey
//
// MotionComponent (lean runtime state)
// Stores current movement state only (what is happening), not gameplay rules (why/how).
// Systems can read/write this during simulation to drive Transform/Rigidbody, animation, etc.

#include "math/Vec3.h"

namespace ecs {

struct MotionComponent {
  enum class Mode {
    Walking,
    Running,
    Crouching,
    Sliding,
    Climbing,
    Dashing,
    Jumping,
    Flying,
    Gliding,
    Swimming,
    Falling,
    Physics,  // driven primarily by a physics simulation
  };

  enum class MovementPhase {
    Idle,
    Starting,
    Moving,
    Stopping,
  };

  Mode mode = Mode::Walking;
  MovementPhase movementPhase = MovementPhase::Idle;

  // Input/intent (unit-ish vector; system decides how to interpret).
  math::Vec3 desiredDirection{0.0f, 0.0f, 0.0f};

  // Intent expressed as an explicit velocity target (units/sec). Movement integrates toward this.
  math::Vec3 desiredVelocity{0.0f, 0.0f, 0.0f};

  // Runtime kinematics (system-integrated).
  math::Vec3 velocity{0.0f, 0.0f, 0.0f};
  math::Vec3 acceleration{0.0f, 0.0f, 0.0f};
  float currentSpeed = 0.0f;

  // Grounding info (filled by movement/physics systems).
  math::Vec3 groundNormal{0.0f, 1.0f, 0.0f};
  bool isMoving = false;
  bool isGrounded = false;
};

}  // namespace ecs
