#include "ecs/systems/GraphicsSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/CameraComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"

#include "math/Vec3.h"
#include "render/Color.h"

#include <cmath>
#include <variant>

namespace ecs::systems {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;

math::Vec3 forwardFromPitchYawDeg(const ecs::TransformComponent& tr) {
  const float pitch = tr.rotation.x * kDegToRad;
  const float yaw = tr.rotation.y * kDegToRad;

  // Convention:
  // - yaw=0, pitch=0 => +Z forward
  // - positive yaw turns toward +X
  // - positive pitch looks downward (screen-space typical), hence -sin(pitch) on Y
  const math::Vec3 fwd{
      std::cos(pitch) * std::sin(yaw),
      -std::sin(pitch),
      std::cos(pitch) * std::cos(yaw),
  };

  return math::normalize(fwd);
}

bool readFloatParam(const ecs::ShaderComponent& shader, const char* name, float& out) {
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<float>(&p.value)) {
      out = *v;
      return true;
    }
  }
  return false;
}

bool readBaseColorParam(const ecs::ShaderComponent& shader, float& r, float& g, float& b) {
  for (const auto& p : shader.parameters) {
    if (p.name != "baseColor") continue;
    if (const auto* c = std::get_if<render::Color>(&p.value)) {
      r = c->r;
      g = c->g;
      b = c->b;
      return true;
    }
  }
  return false;
}

}  // namespace

const GraphicsSystem::FrameSnapshot& GraphicsSystem::tick(EntityRegistry& registry) {
  m_frame.camera = {};
  m_frame.terrains.clear();
  m_frame.lights.clear();

  // --- Camera (pick the first available) ---
  registry.view<ecs::CameraComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::CameraComponent& cam, const ecs::TransformComponent& tr) {
        if (m_frame.camera.entity != ecs::kInvalidEntityId) return;
        m_frame.camera.entity = id;
        m_frame.camera.position = tr.position;
        m_frame.camera.forward = forwardFromPitchYawDeg(tr);
        m_frame.camera.fovYRadians = cam.fieldOfViewDeg * kDegToRad;
        m_frame.camera.nearClip = cam.nearClipPlane;
        m_frame.camera.farClip = cam.farClipPlane;
      });

  // --- Terrains with shaders ---
  registry.view<ecs::TerrainComponent, ecs::ShaderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::TerrainComponent& terrain,
          const ecs::ShaderComponent& shader,
          const ecs::TransformComponent& tr) {
        if (!shader.enabled) return;
        TerrainDraw draw;
        draw.entity = id;
        draw.position = tr.position;
        draw.gridWidth = terrain.gridWidth;
        draw.gridHeight = terrain.gridHeight;
        draw.cellSizeMeters = terrain.cellSizeMeters;
        draw.heightScaleMeters = terrain.heightScaleMeters;
        draw.noise = terrain.noise;
        draw.noiseSeed = static_cast<std::uint32_t>(id) * 1337u;
        draw.shader = shader.shader;
        draw.renderMode = static_cast<int>(shader.renderMode);
        draw.cullMode = static_cast<int>(shader.cullMode);
        draw.depthTest = static_cast<int>(shader.depthTest);
        draw.blendMode = static_cast<int>(shader.blendMode);
        draw.depthWrite = shader.depthWrite;
        draw.doubleSided = shader.doubleSided;
        draw.receiveShadows = shader.receiveShadows;
        draw.castShadows = shader.castShadows;
        draw.textureCount = shader.textures.size();
        draw.parameterCount = shader.parameters.size();

        (void)readBaseColorParam(shader, draw.baseColorR, draw.baseColorG, draw.baseColorB);
        (void)readFloatParam(shader, "roughness", draw.roughness);
        (void)readFloatParam(shader, "metallic", draw.metallic);
        m_frame.terrains.push_back(std::move(draw));
      });

  // --- Lights ---
  registry.view<ecs::LightComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::LightComponent& light, const ecs::TransformComponent& tr) {
        if (!light.enabled) return;
        LightDraw draw;
        draw.entity = id;
        draw.type = static_cast<int>(light.type);
        draw.position = tr.position;
        draw.direction = light.direction;
        draw.color = light.color;
        draw.intensity = light.intensity;
        draw.range = light.range;
        draw.castShadows = light.castShadows;
        m_frame.lights.push_back(std::move(draw));
      });

  return m_frame;
}

}  // namespace ecs::systems
