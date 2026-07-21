#pragma once

// Author: Karl-Johan Bailey
//
// StatsComponent (lightweight)
// Used for actors and destructibles (players, NPCs, enemies, barrels, etc).
// Systems read these values to calculate movement and damage.
//
// Notes:
// - Many values may be 0 to mean "not applicable" for a given entity type.
// - Health reaching 0 is handled by a system (e.g., destroy entity, play death animation, etc).

namespace ecs {

struct StatsComponent {
  // Core survivability.
  float health = 0.0f;
  float maxHealth = 0.0f;

  // Combat stats.
  float defense = 0.0f;
  float baseAttack = 0.0f;
  float specialAttack = 0.0f;

  // Generic speed stat (can be used as a multiplier).
  float speed = 0.0f;

  // Movement speeds (units per second; engine-defined).
  float walkingSpeed = 0.0f;
  float runningSpeed = 0.0f;
  float swimmingSpeed = 0.0f;
};

}  // namespace ecs

