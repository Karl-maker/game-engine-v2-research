#pragma once

// Author: Karl-Johan Bailey
//
// RigidbodyComponent (descriptive only)
// Stores physics body properties. A physics system is responsible for integrating velocities
// and applying the result back to TransformComponent.

#include "math/Vec3.h"

namespace ecs {

struct BuoyantComponent {


  bool buoyant = true;
  bool sway = false;
  float buoyancyHeight = -0.5;
};

}  // namespace ecs