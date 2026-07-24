#pragma once

// Author: Karl-Johan Bailey

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include <iostream>
namespace ecs::services {

struct CharacterConfig {
  // Character mesh/asset reference.
  std::string meshReference;

  // Socket/bone keys.
  std::string leftHandKey = "left_hand";
  std::string rightHandKey = "right_hand";
  std::string headKey = "head";
};

class CharacterFactory final {
 public:
  EntityId create(
      EntityRegistry& registry,
      const CharacterConfig& config);
};

}  // namespace ecs::services