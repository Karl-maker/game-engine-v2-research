#pragma once

// Author: Karl-Johan Bailey
//
// Gameplay demo:
// - Spawns a camera, a terrain entity with a shader, and a light.
// - Runs GraphicsSystem each tick and prints a small render snapshot periodically.

#include "core/IGame.h"
#include "core/ControlService.h"
#include "core/FrameDebugger.h"
#include "core/ThreadService.h"

#include "ecs/EntityId.h"
#include "ecs/EntityRegistry.h"
#include "assets/MeshAssetService.h"
#include "ecs/services/EventService.h"
#include "ecs/systems/ControllerSystem.h"
#include "ecs/systems/AnimationSystem.h"
#include "ecs/systems/AttachmentSystem.h"
#include "ecs/systems/AudioSystem.h"
#include "ecs/systems/CollisionDetectionSystem.h"
#include "ecs/systems/CollisionResolutionSystem.h"
#include "ecs/systems/HitDetectionSystem.h"
#include "ecs/systems/HierarchySystem.h"
#include "ecs/systems/GraphicsSystem.h"
#include "ecs/systems/HudSystem.h"
#include "ecs/systems/GravitySystem.h"
#include "ecs/systems/IdleAnimationSystem.h"
#include "ecs/systems/IKSystem.h"
#include "ecs/systems/JumpSystem.h"
#include "ecs/systems/MotionSystem.h"
#include "ecs/systems/MovementSystem.h"
#include "ecs/systems/PoseSystem.h"
#include "ecs/systems/RayDetectionSystem.h"
#include "ecs/systems/SensorSystem.h"
#include "ecs/systems/SkeletonAssetSyncSystem.h"
#include "ecs/systems/SkyPresetSystem.h"
#include "ecs/systems/SocketSystem.h"
#include "ecs/systems/ThirdPersonCameraSystem.h"
#include "ecs/services/ChunkStreamingService.h"
#include "ecs/services/EntityFactoryRegistry.h"
#include "ecs/services/IChunkSource.h"

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
#include "graphics/OpenGlRenderer.h"
#endif

#include <string>
#include <memory>

namespace games {

struct GameplayDemoConfig final {
  int windowWidth = 1280;
  int windowHeight = 720;
  bool fullscreen = false;
  bool vsync = false;
  int fullscreenRefreshRateHz = 144;  // 0 = platform default; 144 is a good high-refresh default

  std::string chunkConfigPath = "assets/world/chunks_demo.json";
  float chunkSizeMeters = 96.0f;
  int chunkSearchRadius = 2;
  float chunkLoadProximityMeters = 14.0f;
  float chunkUnloadProximityMeters = 22.0f;
};

class GameplayDemoGame final : public core::IGame {
 public:
  explicit GameplayDemoGame(GameplayDemoConfig config = {}) : m_config(config) {}
  void onStart() override;
  void onTick(const core::TickContext& ctx) override;
  void onStop() override;

 private:
  GameplayDemoConfig m_config{};
 ecs::EntityRegistry m_registry;
  core::ControlService m_controls;
  core::FrameDebugger m_frameDebugger;
  core::ThreadService m_threads;
  assets::MeshAssetService m_meshAssets;
  ecs::services::EventService m_events;
  ecs::systems::ControllerSystem m_controllerSystem;
  ecs::systems::AnimationSystem m_animationSystem;
  ecs::systems::AudioSystem m_audioSystem;
  ecs::systems::IdleAnimationSystem m_idleAnimationSystem;
  ecs::systems::IKSystem m_ikSystem;
  ecs::systems::AttachmentSystem m_attachmentSystem;
  ecs::systems::GravitySystem m_gravitySystem;
  ecs::systems::JumpSystem m_jumpSystem;
  ecs::systems::CollisionDetectionSystem m_collisionDetectionSystem;
  ecs::systems::CollisionResolutionSystem m_collisionResolutionSystem;
  ecs::systems::HitDetectionSystem m_hitDetectionSystem;
  ecs::systems::HierarchySystem m_hierarchySystem;
  ecs::systems::MotionSystem m_motionSystem;
  ecs::systems::MovementSystem m_movementSystem;
  ecs::systems::PoseSystem m_poseSystem;
  ecs::systems::SocketSystem m_socketSystem;
  ecs::systems::RayDetectionSystem m_rayDetectionSystem;
  ecs::systems::SensorSystem m_sensorSystem;
  ecs::systems::SkeletonAssetSyncSystem m_skeletonAssetSyncSystem;
  ecs::systems::SkyPresetSystem m_skyPresets;
  ecs::systems::ThirdPersonCameraSystem m_thirdPersonCameraSystem;
  ecs::systems::GraphicsSystem m_graphics;
  ecs::systems::HudSystem m_hudSystem;
  ecs::services::EntityFactoryRegistry m_factoryRegistry;
  ecs::services::ChunkStreamingService m_chunkStreaming;
  std::unique_ptr<ecs::services::IChunkSource> m_chunkSource;

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  graphics::OpenGlRenderer m_renderer;
#endif

  ecs::EntityId m_camera = ecs::kInvalidEntityId;
  ecs::EntityId m_player = ecs::kInvalidEntityId;
  ecs::EntityId m_demoNpc = ecs::kInvalidEntityId;
  ecs::EntityId m_terrain = ecs::kInvalidEntityId;
  ecs::EntityId m_light = ecs::kInvalidEntityId;
  ecs::EntityId m_renderSettings = ecs::kInvalidEntityId;
  ecs::EntityId m_hud = ecs::kInvalidEntityId;
  ecs::EntityId m_playerHandSocket = ecs::kInvalidEntityId;
  ecs::EntityId m_playerHitVolume = ecs::kInvalidEntityId;
  ecs::EntityId m_demoNpcHurtVolume = ecs::kInvalidEntityId;

  double m_printTimer = 0.0;
  double m_attackTimerSeconds = 0.0;
  std::string m_debugOverlayText;
};

}  // namespace games
