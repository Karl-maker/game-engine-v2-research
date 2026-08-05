#pragma once

// Author: Karl-Johan Bailey
//
// Tooling demo:
// - Spawns a free camera, sky/light/fog, and chunk-streamed world content.
// - Runs only the graphics snapshot path plus chunk streaming for an editor-like view.

#include "core/ControlService.h"
#include "core/FrameDebugger.h"
#include "core/IGame.h"
#include "core/ThreadService.h"
#include "games/Game.h"
#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "assets/MeshAssetService.h"
#include "ecs/services/ChunkStreamingService.h"
#include "ecs/services/EntityFactoryRegistry.h"
#include "ecs/services/FileChunkSource.h"
#include "ecs/systems/GraphicsSystem.h"
#include "ecs/systems/SkeletonAssetSyncSystem.h"
#include "ecs/systems/SkyPresetSystem.h"
#include "graphics/OpenGlRenderer.h"
#include "math/Vec3.h"

#include <filesystem>
#include <memory>
#include <string>

namespace games {

class Tooling final : public core::IGame {
 public:
  explicit Tooling(const GameConfig& config = {}) : m_config(config) {}
  void onStart() override;
  void onTick(const core::TickContext& ctx) override;
  void onStop() override;

 private:
  void applyToolingConfigToRuntime();
  bool applyPersistentWorldConfigToScene();
  bool reloadToolingConfigIfChanged();
  bool loadToolingConfig();

  GameConfig m_config{};
  std::string m_toolingConfigPath = "assets/world/tooling_view.json";
  std::filesystem::file_time_type m_toolingConfigWriteTime{};
  std::filesystem::file_time_type m_worldConfigWriteTime{};
  math::Vec3 m_cameraStartPosition{0.0f, 36.0f, 120.0f};
  math::Vec3 m_cameraStartRotation{24.0f, 0.0f, 0.0f};
  ecs::EntityRegistry m_registry;
  core::ControlService m_controls;
  core::FrameDebugger m_frameDebugger;
  core::ThreadService m_threads;
  assets::MeshAssetService m_meshAssets;
  ecs::systems::GraphicsSystem m_graphics;
  ecs::services::EntityFactoryRegistry m_factoryRegistry;
  ecs::services::ChunkStreamingService m_chunkStreaming;
  std::unique_ptr<ecs::services::FileChunkSource> m_chunkSource;
  graphics::OpenGlRenderer m_renderer;

  ecs::EntityId m_camera = ecs::kInvalidEntityId;
  ecs::EntityId m_renderSettings = ecs::kInvalidEntityId;
  ecs::EntityId m_light = ecs::kInvalidEntityId;
  ecs::EntityId m_sky = ecs::kInvalidEntityId;
  ecs::EntityId m_fog = ecs::kInvalidEntityId;
  ecs::systems::SkeletonAssetSyncSystem m_skeletonAssetSyncSystem;
  ecs::systems::SkyPresetSystem m_skyPresetSystem;

  std::string m_debugOverlayText;
};

}  // namespace games
