#pragma once

#include "../EntityId.h"

#include <string>

namespace ecs {

struct InteractiveComponent {
  float interactive_distance = 1.0f;

  // Per-frame detection/debug state filled by InteractionDetectionSystem.
  bool detected_this_frame = false;
  bool detected_by_player_sensor = false;
  EntityId source_sensor_entity = kInvalidEntityId;
  EntityId source_parent_entity = kInvalidEntityId;
  EntityId source_ray_entity = kInvalidEntityId;
  float source_distance = 0.0f;
  std::string source_sensor_name;
  std::string source_parent_name;
  std::string source_raycast_category;
};

}  // namespace ecs
