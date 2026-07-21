#pragma once

// Author: Karl-Johan Bailey
//
// TransformComponent
// Any element can have a location/orientation/scale in the world.
// Attach it to an entity to give it spatial data.

namespace ecs {

struct TransformComponent {
  struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
  } position;

  struct Rotation {
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
  } rotation;

  struct Scale {
    float x = 1.0f;
    float y = 1.0f;
    float z = 1.0f;
  } scale;
};

}  // namespace ecs

