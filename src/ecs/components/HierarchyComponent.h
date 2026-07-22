#pragma once

// Author: Karl-Johan Bailey
//
// HierarchyComponent (descriptive ownership/parenting)
// Allows entities to own each other in a parent/child tree.
//
// Intended behavior (applied by a transform hierarchy system):
// - Child world transform is derived from parent world transform + child's local offset
// - Child keeps its own local offset (position/rotation/scale) relative to the parent
//
// Typical setup:
// - Parent entity has HierarchyComponent listing its children
// - Child entity has HierarchyComponent::parentEntity set to the parent
//
// Notes:
// - This component stores relationship + local offset only.
// - A dedicated system should compute/copy world transforms each frame.

#include "ecs/EntityId.h"
#include "math/Vec3.h"

#include <vector>

namespace ecs {

struct HierarchyComponent {
  bool enabled = true;

  // Parent this entity is attached to (kInvalidEntityId if none/root).
  EntityId parentEntity = kInvalidEntityId;

  // Children owned by this entity.
  // Kept here so gameplay can quickly enumerate owned entities (weapons, sensors, etc).
  std::vector<EntityId> children;

  // Local transform offset relative to the parent.
  math::Vec3 localPosition{0.0f, 0.0f, 0.0f};
  math::Vec3 localRotation{0.0f, 0.0f, 0.0f};  // pitch/yaw/roll in degrees
  math::Vec3 localScale{1.0f, 1.0f, 1.0f};

  // Inheritance flags (system-defined).
  bool inheritPosition = true;
  bool inheritRotation = true;
  bool inheritScale = true;
};

}  // namespace ecs

