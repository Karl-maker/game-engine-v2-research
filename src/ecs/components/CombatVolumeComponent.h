#pragma once

// Author: Karl-Johan Bailey
//
// CombatVolumeComponent (descriptive only)
// Defines hit/hurt volumes for combat systems (damage, hit detection, etc).
//
// Typical usage:
// - Main character has multiple Hurt volumes (body, head)
// - Weapon (e.g., sword) has a Hit volume that is enabled during attack windows

#include "math/Vec3.h"
#include "ecs/EntityId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ecs {

struct CombatVolumeComponent {
  enum class Role {
    Hurt,  // receives damage
    Hit,   // deals damage
  };

  enum class Shape {
    Sphere,
    Capsule,
    Box,
  };

  struct BoxSize {
    float width = 0.0f;
    float height = 0.0f;
    float length = 0.0f;
  };

  struct CapsuleSize {
    float radius = 0.0f;
    float height = 0.0f;
  };

  struct SphereSize {
    float radius = 0.0f;
  };

  struct Volume {
    Role role = Role::Hurt;
    Shape shape = Shape::Sphere;

    math::Vec3 offset{0.0f, 0.0f, 0.0f};

    // Shape data (interpret based on `shape`).
    BoxSize box;
    CapsuleSize capsule;
    SphereSize sphere;

    // Hurt volume fields:
    float damageMultiplier = 1.0f;

    // Hit volume fields:
    float damage = 0.0f;
    std::string damageType;
    float force = 0.0f;

    bool enabled = true;
    bool singleHit = false;

    std::vector<std::string> tags;
  };

  // Multiple volumes per entity (body, head, sword hitbox, etc).
  EntityId ownerEntity = kInvalidEntityId;
  std::string attachmentKey;
  std::string sourceMeshId;
  std::vector<Volume> volumes;
};

}  // namespace ecs
