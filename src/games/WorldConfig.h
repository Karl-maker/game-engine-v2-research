#pragma once

#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/RenderSettingsComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/factories/PlayableCharacterFactory.h"

#include <string>

namespace games {

struct PersistentWorldConfig final {
  bool hasPlayer = false;
  ecs::services::PlayableCharacterConfig player{};

  bool hasSky = false;
  ecs::SkyComponent sky{};

  bool hasFog = false;
  ecs::FogVolumeComponent fog{};
  math::Vec3 fogAnchor{0.0f, 4.0f, 0.0f};

  bool hasRenderSettings = false;
  ecs::RenderSettingsComponent renderSettings{};

  bool hasSun = false;
  ecs::LightComponent sun{};
};

bool loadPersistentWorldConfig(const std::string& chunkConfigPath, PersistentWorldConfig& outConfig);

}  // namespace games
