#pragma once

// Author: Karl-Johan Bailey
//
// PhysicsMaterialComponent (descriptive only)
// Describes surface behavior for physics interactions.
//
// Examples:
// - Ice:    friction=0.02, restitution=0.0
// - Rubber: friction=1.0,  restitution=0.8

namespace ecs {

struct PhysicsMaterialComponent {
  // Sliding friction coefficient.
  float friction = 0.5f;

  // Restitution (bounce) coefficient.
  float restitution = 0.0f;

  // Rolling friction coefficient.
  float rollingFriction = 0.0f;

  // Optional density (kg/m^3 or engine-defined units). If not set, use default.
  bool hasDensity = false;
  float density = 0.0f;
};

}  // namespace ecs

