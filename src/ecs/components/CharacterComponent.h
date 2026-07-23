#pragma once

// Author: Karl-Johan Bailey

namespace ecs {

struct CharacterComponent final {
  bool enabled = true;
  float turnSpeedDegPerSecond = 720.0f;
};

}  // namespace ecs
