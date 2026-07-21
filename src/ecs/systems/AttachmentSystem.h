#pragma once

// Author: Karl-Johan Bailey
//
// AttachmentSystem
// Applies AttachmentComponent descriptors to entity transforms.
// This is intentionally small and demo-focused; expand as you add real gameplay needs.

#include "ecs/EntityRegistry.h"

namespace ecs::systems {

class AttachmentSystem final {
 public:
  // Updates entity transforms based on their attachment descriptors.
  // Call this once per frame, after gameplay has queued/appended attachment changes.
  void update(EntityRegistry& registry, double deltaSeconds);
};

}  // namespace ecs::systems

