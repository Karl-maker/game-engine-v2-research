// Author: Karl-Johan Bailey

#include "games/Tooling.h"
#include "games/WorldConfig.h"

#include "core/TickContext.h"

#include "ecs/components/CameraComponent.h"
#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/RenderSettingsComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/components/TransformComponent.h"
#include "data/JsonUtil.h"
#include "ecs/factories/FactoryKeyService.h"
#include "ecs/services/FileChunkSource.h"
#include "materials/presets/RealisticSkyClouds.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace games {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;

math::Vec3 crossVec3(const math::Vec3& a, const math::Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

math::Vec3 normalizeSafe(const math::Vec3& v) {
  const float len = math::length(v);
  if (len <= 0.000001f) return {0.0f, 0.0f, 0.0f};
  return v * (1.0f / len);
}

math::Vec3 forwardFromPitchYawDeg(float pitchDeg, float yawDeg) {
  const float pitch = pitchDeg * kDegToRad;
  const float yaw = yawDeg * kDegToRad;
  return normalizeSafe({std::cos(pitch) * std::sin(yaw), -std::sin(pitch), std::cos(pitch) * std::cos(yaw)});
}

bool readVec3OrDefault(const data::JsonValue::Object& obj, const char* key, math::Vec3& out) {
  if (const auto* v = data::getObjectKey(obj, key)) {
    return data::readVec3(*v, out);
  }
  return false;
}

std::string formatSceneCounts(const ecs::systems::GraphicsSystem::FrameSnapshot& frame) {
  std::ostringstream out;
  out << "scene: terrains=" << frame.terrains.size();
  out << " meshes=" << frame.meshes.size();
  out << " grasses=" << frame.grasses.size();
  out << " rocks=" << frame.rocks.size();
  out << " billboards=" << frame.billboards.size();
  out << " vfx=" << frame.vfx.size();
  out << " rays=" << frame.rays.size();
  out << " lines=" << frame.debugLines.size();
  return out.str();
}

}  // namespace

void Tooling::applyToolingConfigToRuntime() {
  ecs::services::ChunkStreamingConfig chunkCfg{};
  chunkCfg.chunkSizeMeters = m_config.chunkSizeMeters;
  chunkCfg.searchRadiusChunks = m_config.chunkSearchRadius;
  chunkCfg.loadProximityMeters = m_config.chunkLoadProximityMeters;
  chunkCfg.unloadProximityMeters = m_config.chunkUnloadProximityMeters;
  m_chunkStreaming.setConfig(chunkCfg);
}

bool Tooling::applyPersistentWorldConfigToScene() {
  PersistentWorldConfig persistentConfig{};
  if (!loadPersistentWorldConfig(m_config.chunkConfigPath, persistentConfig)) return false;

  if (auto* sky = m_registry.tryGet<ecs::SkyComponent>(m_sky)) {
    *sky = persistentConfig.sky;
    sky->linkedDirectionalLightEntity = m_light;
    sky->linkedFogVolumeEntity =
        (!persistentConfig.hasFog && m_fog != ecs::kInvalidEntityId) ? m_fog : ecs::kInvalidEntityId;
  }

  if (auto* fogTransform = m_registry.tryGet<ecs::TransformComponent>(m_fog)) {
    fogTransform->position = persistentConfig.fogAnchor;
  }
  if (auto* fog = m_registry.tryGet<ecs::FogVolumeComponent>(m_fog)) {
    *fog = persistentConfig.fog;
  }

  m_skyPresetSystem.tick(m_registry);

  std::error_code ec;
  const auto wt = std::filesystem::last_write_time(m_config.chunkConfigPath, ec);
  if (!ec) m_worldConfigWriteTime = wt;
  return true;
}

bool Tooling::loadToolingConfig() {
  if (m_toolingConfigPath.empty()) return false;

  std::ifstream f(m_toolingConfigPath);
  if (!f.is_open()) return false;

  std::stringstream ss;
  ss << f.rdbuf();
  const std::string text = ss.str();
  if (text.empty()) return false;

  auto parsed = data::parseJson(text);
  if (!parsed.ok) {
    std::cerr << "[tooling] JSON parse error at " << parsed.errorOffset << " in " << m_toolingConfigPath << ": " << parsed.error
              << "\n";
    return false;
  }

  const auto* root = parsed.value.tryObject();
  if (!root) return false;

  if (const auto* v = data::getObjectKey(*root, "chunkConfigPath")) {
    std::string path;
    if (data::readString(*v, path) && !path.empty()) {
      m_config.chunkConfigPath = std::move(path);
    }
  }
  m_config.chunkSizeMeters = data::getFloatOr(*root, "chunkSizeMeters", m_config.chunkSizeMeters);
  m_config.chunkSearchRadius = std::max(0, data::getIntOr(*root, "chunkSearchRadius", m_config.chunkSearchRadius));
  m_config.chunkLoadProximityMeters = data::getFloatOr(*root, "chunkLoadProximityMeters", m_config.chunkLoadProximityMeters);
  m_config.chunkUnloadProximityMeters = data::getFloatOr(*root, "chunkUnloadProximityMeters", m_config.chunkUnloadProximityMeters);
  (void)readVec3OrDefault(*root, "cameraStartPosition", m_cameraStartPosition);
  (void)readVec3OrDefault(*root, "cameraStartRotation", m_cameraStartRotation);

  std::error_code ec;
  const auto wt = std::filesystem::last_write_time(m_toolingConfigPath, ec);
  if (!ec) m_toolingConfigWriteTime = wt;

  applyToolingConfigToRuntime();
  return true;
}

bool Tooling::reloadToolingConfigIfChanged() {
  if (m_toolingConfigPath.empty()) return false;

  std::error_code ec;
  const auto wt = std::filesystem::last_write_time(m_toolingConfigPath, ec);
  if (ec) return false;
  if (m_toolingConfigWriteTime != std::filesystem::file_time_type{} && wt == m_toolingConfigWriteTime) return false;
  return loadToolingConfig();
}

void Tooling::onStart() {
  std::cout << "\x1B[2J\x1B[H";
  std::cout << "Tooling mode (editor view)\n";
  std::cout << "- Free camera + graphics snapshot only\n";
  std::cout << "- Press Esc to release/recapture the mouse\n";
  std::cout << "- Edit the chunk JSON while the view stays live\n";
  std::cout << "- Edit " << m_toolingConfigPath << " for camera + chunk range settings\n";
  std::cout << "Controls: WASD move, mouse look, Space up, Ctrl down, Shift faster\n";
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  std::cout << "- Opens an OpenGL window and renders the world view\n";
#else
  std::cout << "- Runs GraphicsSystem each tick (prints a snapshot)\n";
#endif
  std::cout << "Type `q` then Enter to quit.\n\n";

  m_meshAssets.start();

  (void)loadToolingConfig();

  m_camera = m_registry.createEntity("tool_camera");
  auto& camTr = m_registry.emplace<ecs::TransformComponent>(m_camera);
  camTr.position = m_cameraStartPosition;
  camTr.rotation = m_cameraStartRotation;
  auto& cam = m_registry.emplace<ecs::CameraComponent>(m_camera);
  cam.renderScale = 1.0f;
  cam.depthOfField.enabled = false;
  cam.motionBlur.enabled = false;

  m_renderSettings = m_registry.createEntity("render_settings");
  auto& rs = m_registry.emplace<ecs::RenderSettingsComponent>(m_renderSettings);
  rs.enabled = true;
  rs.shadowsEnabled = true;
  rs.shadowQuality = 1;
  rs.shadowStrength = 0.55f;
  rs.shadowUseTessellation = true;
  rs.showRays = true;
  rs.showCollisionBoxes = true;
  rs.showCombatBoxes = true;
  rs.showSkeletonBones = true;

  m_light = m_registry.createEntity("sun");
  m_registry.emplace<ecs::TransformComponent>(m_light);
  {
    auto& light = m_registry.emplace<ecs::LightComponent>(m_light);
    light.type = ecs::LightComponent::Type::Directional;
    light.direction = math::normalize(math::Vec3{-0.35f, -1.0f, -0.15f});
    light.intensity = 3.25f;
    light.color = {1.0f, 0.96f, 0.88f};
    light.castShadows = true;
  }

  m_sky = m_registry.createEntity("sky");
  m_registry.emplace<ecs::TransformComponent>(m_sky);
  {
    m_registry.emplace<ecs::SkyComponent>(m_sky);
    auto& sh = m_registry.emplace<ecs::ShaderComponent>(m_sky, materials::presets::RealisticSkyClouds());
    sh.shader.key = "graphics/shaders/sky";
  }

  m_fog = m_registry.createEntity("mist");
  {
    auto& tr = m_registry.emplace<ecs::TransformComponent>(m_fog);
    tr.position = {0.0f, 4.0f, 0.0f};
    m_registry.emplace<ecs::FogVolumeComponent>(m_fog);
  }
  (void)applyPersistentWorldConfigToScene();

  ecs::services::registerFactoriesFromEcsFactoriesDir(m_factoryRegistry);
  applyToolingConfigToRuntime();
  m_chunkSource = std::make_unique<ecs::services::FileChunkSource>(m_config.chunkConfigPath);

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  graphics::OpenGlRenderer::WindowConfig wcfg{};
  wcfg.width = m_config.windowWidth;
  wcfg.height = m_config.windowHeight;
  wcfg.fullscreen = m_config.fullscreen;
  wcfg.vsync = m_config.vsync;
  wcfg.refreshRateHz = m_config.fullscreenRefreshRateHz;
  if (!m_renderer.start(wcfg, "Duppy - Tooling View")) {
    std::cerr << "OpenGL renderer failed to start; falling back to terminal snapshot.\n";
  }
#endif
}

void Tooling::onTick(const core::TickContext& ctx) {
  m_frameDebugger.beginFrame();

  for (const auto& line : ctx.inputLines) {
    if (line == "q" || line == "quit" || line == "exit") {
      if (ctx.requestQuit) ctx.requestQuit();
      return;
    }
  }

  if (m_chunkSource && m_chunkSource->reloadIfChanged()) {
    (void)applyPersistentWorldConfigToScene();
    m_chunkStreaming.unloadAll(m_registry);
    std::cout << "[chunks] reloaded " << m_chunkSource->path() << "\n";
  }

  if (reloadToolingConfigIfChanged()) {
    if (auto* camTr = m_registry.tryGet<ecs::TransformComponent>(m_camera)) {
      camTr->position = m_cameraStartPosition;
      camTr->rotation = m_cameraStartRotation;
    }
    if (!m_chunkSource || m_chunkSource->path() != m_config.chunkConfigPath) {
      m_chunkSource = std::make_unique<ecs::services::FileChunkSource>(m_config.chunkConfigPath);
    }
    (void)applyPersistentWorldConfigToScene();
    m_chunkStreaming.unloadAll(m_registry);
    std::cout << "[tooling] reloaded " << m_toolingConfigPath << "\n";
  }

  core::ControlService::RealtimeInput rt{};
  bool mouseCaptured = false;
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  if (m_renderer.isOpen()) {
    m_renderer.pollEvents();
    const auto in = m_renderer.drainRealtimeInput();
    mouseCaptured = in.lookActive;
    rt.hasInput = true;
    rt.moveX = in.moveX;
    rt.moveZ = in.moveZ;
    rt.sprint = in.sprint;
    rt.crouch = in.crouch;
    rt.jump = in.jump;
    rt.lookActive = in.lookActive;
    constexpr float kMouseToDeg = 0.08f;
    rt.lookDeltaYawDeg = in.mouseDx * kMouseToDeg;
    rt.lookDeltaPitchDeg = in.mouseDy * kMouseToDeg;
  }
#endif
  m_controls.update(ctx, rt.hasInput ? &rt : nullptr);

  if (auto* camTr = m_registry.tryGet<ecs::TransformComponent>(m_camera)) {
    const auto& state = m_controls.state();
    camTr->rotation.x = std::clamp(camTr->rotation.x + state.lookDeltaDeg.x, -89.0f, 89.0f);
    camTr->rotation.y += state.lookDeltaDeg.y;

    const math::Vec3 forward = forwardFromPitchYawDeg(camTr->rotation.x, camTr->rotation.y);
    math::Vec3 right = crossVec3({0.0f, 1.0f, 0.0f}, forward);
    if (math::length(right) <= 0.000001f) {
      right = {1.0f, 0.0f, 0.0f};
    } else {
      right = normalizeSafe(right);
    }
    const float speed = state.sprint ? 30.0f : 15.0f;
    camTr->position = camTr->position + (forward * (state.moveDirection.z * speed * ctx.deltaSeconds));
    camTr->position = camTr->position + (right * (state.moveDirection.x * speed * ctx.deltaSeconds));
    if (state.jump) camTr->position.y = camTr->position.y + (speed * ctx.deltaSeconds);
    if (state.crouch) camTr->position.y = camTr->position.y - (speed * ctx.deltaSeconds);

    // Keep the camera roughly over the demo terrain so the first load is visible.
    camTr->position.y = std::clamp(camTr->position.y, 1.0f, 120.0f);
  }

  if (m_chunkSource) {
    m_chunkStreaming.tick(m_registry, m_camera, *m_chunkSource, m_factoryRegistry);
  }

  m_skeletonAssetSyncSystem.tick(m_registry, m_meshAssets);
  m_skyPresetSystem.tick(m_registry);

  const auto& frame = m_graphics.tick(m_registry);
  std::ostringstream overlay;
  overlay << formatSceneCounts(frame);
  overlay << "\nmouse=" << (mouseCaptured ? "captured" : "released");
  overlay << "\nchunks radius=" << m_config.chunkSearchRadius << " load=" << m_config.chunkLoadProximityMeters;
  overlay << "\ntooling=" << m_toolingConfigPath;
  overlay << "\nfile=" << m_config.chunkConfigPath;
  m_debugOverlayText = overlay.str();

#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  if (m_renderer.isOpen()) {
    m_renderer.render(frame,
                      true,
                      static_cast<float>(ctx.fpsEstimate),
                      static_cast<float>(ctx.deltaSeconds * 1000.0),
                      static_cast<float>(ctx.cpuWorkSeconds * 1000.0),
                      m_debugOverlayText);
  } else {
    if (ctx.requestQuit) ctx.requestQuit();
    return;
  }
#endif
}

void Tooling::onStop() {
#if defined(DUPPY_ENABLE_OPENGL) && DUPPY_ENABLE_OPENGL
  m_renderer.stop();
#endif
  if (m_chunkSource) {
    m_chunkStreaming.unloadAll(m_registry);
    m_chunkSource.reset();
  }
  m_meshAssets.stop();
  std::cout << "\nStopped tooling demo.\n";
}

}  // namespace games
