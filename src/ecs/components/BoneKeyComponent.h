#pragma once

// Author: Karl-Johan Bailey

#include <iostream>

namespace ecs {

struct BoneKeyComponent final {
  // Socket/bone keys.
  std::string leftHandKey = "left_hand";
  std::string rightHandKey = "right_hand";
  std::string headKey = "head";
};

}  // namespace ecs