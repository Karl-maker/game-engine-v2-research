#pragma once

#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/RenderSettingsComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/factories/PlayableCharacterFactory.h"
#include "ecs/services/ChunkStreamingService.h"

#include <string>

namespace games {

struct PerformanceWorldConfig final {
  std::string preset = "custom";
  bool applyToTooling = true;
};

struct ProfilingWorldConfig final {
  bool overlayEnabled = false;
  bool verbose = true;
  bool showMaxSamples = false;
  int cpuTopCount = 8;
  int renderTopCount = 6;
  bool showSceneCounts = true;
  bool showPerformanceHints = true;
};

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

  bool hasChunkStreaming = false;
  ecs::services::ChunkStreamingConfig chunkStreaming{};

  bool hasPerformance = false;
  PerformanceWorldConfig performance{};

  bool hasProfiling = false;
  ProfilingWorldConfig profiling{};
};

bool loadPersistentWorldConfig(const std::string& chunkConfigPath, PersistentWorldConfig& outConfig);

}  // namespace games
