#pragma once

#include "ecs/components/FogVolumeComponent.h"

#include <string>

namespace games {

// Loads optional environment fog settings from the world chunk config.
// Expected shape:
// {
//   "environment": {
//     "fog": { ... }
//   }
// }
bool loadEnvironmentFogConfig(const std::string& chunkConfigPath, ecs::FogVolumeComponent& fog);

}  // namespace games
