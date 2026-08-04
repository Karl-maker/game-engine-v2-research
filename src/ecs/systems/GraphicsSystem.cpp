#include "ecs/systems/GraphicsSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/CameraComponent.h"
#include "ecs/components/BillboardComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/CombatVolumeComponent.h"
#include "ecs/components/Draw2DComponent.h"
#include "ecs/components/FogVolumeComponent.h"
#include "ecs/components/GrassPatchComponent.h"
#include "ecs/components/LightComponent.h"
#include "ecs/components/MeshComponent.h"
#include "ecs/components/RenderSettingsComponent.h"
#include "ecs/components/RockScatterComponent.h"
#include "ecs/components/RaycastComponent.h"
#include "ecs/components/VfxComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/SkeletonComponent.h"
#include "ecs/components/SkyComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/services/SpatialHashGridService.h"

#include "math/Vec3.h"
#include "render/Color.h"
#include "render/TextureBinding.h"

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

math::Vec3 forwardFromRotationDeg(const math::Vec3& rotation) {
  const float pitch = rotation.x * kDegToRad;
  const float yaw = rotation.y * kDegToRad;
  return math::normalize(math::Vec3{std::cos(pitch) * std::sin(yaw), -std::sin(pitch), std::cos(pitch) * std::cos(yaw)});
}

math::Vec3 rotateVector(const math::Vec3& v, const math::Vec3& rotation) {
  const float rx = rotation.x * kDegToRad;
  const float ry = rotation.y * kDegToRad;
  const float rz = rotation.z * kDegToRad;

  const float cx = std::cos(rx);
  const float sx = std::sin(rx);
  const float cy = std::cos(ry);
  const float sy = std::sin(ry);
  const float cz = std::cos(rz);
  const float sz = std::sin(rz);

  math::Vec3 out = v;
  out = {out.x, out.y * cx - out.z * sx, out.y * sx + out.z * cx};
  out = {out.x * cy + out.z * sy, out.y, -out.x * sy + out.z * cy};
  out = {out.x * cz - out.y * sz, out.x * sz + out.y * cz, out.z};
  return out;
}

math::Mat4 composeWorld(const ecs::TransformComponent& tr) {
  return math::mul(math::translate(tr.position),
                   math::mul(math::rotateY(tr.rotation.y * kDegToRad),
                             math::mul(math::rotateX(tr.rotation.x * kDegToRad),
                                       math::mul(math::rotateZ(tr.rotation.z * kDegToRad), math::scale(tr.scale)))));
}

math::Mat4 composeTransform(const math::Vec3& position, const math::Vec3& rotation, const math::Vec3& scale) {
  return math::mul(math::translate(position),
                   math::mul(math::rotateY(rotation.y * kDegToRad),
                             math::mul(math::rotateX(rotation.x * kDegToRad),
                                       math::mul(math::rotateZ(rotation.z * kDegToRad), math::scale(scale)))));
}

bool sameVec3(const math::Vec3& a, const math::Vec3& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

math::Vec3 translationFromMat4(const math::Mat4& m) { return {m.m[12], m.m[13], m.m[14]}; }

void addLine(std::vector<GraphicsSystem::FrameSnapshot::DebugLine>& out,
             const math::Vec3& a,
             const math::Vec3& b,
             const render::Color& color) {
  out.push_back({a, b, color});
}

void addAabbLines(std::vector<GraphicsSystem::FrameSnapshot::DebugLine>& out,
                  const ecs::services::SpatialHashGridService::Aabb& bounds,
                  const render::Color& color) {
  const math::Vec3 min = bounds.min;
  const math::Vec3 max = bounds.max;
  const math::Vec3 p000{min.x, min.y, min.z};
  const math::Vec3 p001{min.x, min.y, max.z};
  const math::Vec3 p010{min.x, max.y, min.z};
  const math::Vec3 p011{min.x, max.y, max.z};
  const math::Vec3 p100{max.x, min.y, min.z};
  const math::Vec3 p101{max.x, min.y, max.z};
  const math::Vec3 p110{max.x, max.y, min.z};
  const math::Vec3 p111{max.x, max.y, max.z};

  addLine(out, p000, p001, color);
  addLine(out, p000, p010, color);
  addLine(out, p000, p100, color);
  addLine(out, p001, p011, color);
  addLine(out, p001, p101, color);
  addLine(out, p010, p011, color);
  addLine(out, p010, p110, color);
  addLine(out, p100, p101, color);
  addLine(out, p100, p110, color);
  addLine(out, p011, p111, color);
  addLine(out, p101, p111, color);
  addLine(out, p110, p111, color);
}

ecs::services::SpatialHashGridService::Aabb colliderAabb(const ecs::TransformComponent& tr, const ecs::ColliderComponent& c) {
  const math::Vec3 center = tr.position + c.offset;
  math::Vec3 half{};
  switch (c.shape) {
    case ecs::ColliderComponent::Shape::Sphere:
      half = {c.size.x, c.size.x, c.size.x};
      break;
    case ecs::ColliderComponent::Shape::Capsule:
      half = {c.size.x, c.size.y * 0.5f, c.size.x};
      break;
    case ecs::ColliderComponent::Shape::Terrain:
    case ecs::ColliderComponent::Shape::Box:
    case ecs::ColliderComponent::Shape::Mesh:
    default:
      half = {std::max(0.01f, c.size.x * 0.5f), std::max(0.01f, c.size.y * 0.5f), std::max(0.01f, c.size.z * 0.5f)};
      break;
  }
  return {center - half, center + half};
}

ecs::services::SpatialHashGridService::Aabb combatAabb(const ecs::TransformComponent& tr, const ecs::CombatVolumeComponent::Volume& volume) {
  const math::Vec3 localOffset = rotateVector(volume.offset, tr.rotation);
  const math::Vec3 center = tr.position + localOffset;
  const math::Vec3 scale{std::max(0.01f, tr.scale.x), std::max(0.01f, tr.scale.y), std::max(0.01f, tr.scale.z)};
  switch (volume.shape) {
    case ecs::CombatVolumeComponent::Shape::Sphere: {
      const float radius = std::max(0.01f, volume.sphere.radius * std::max({scale.x, scale.y, scale.z}));
      return {center - math::Vec3{radius, radius, radius}, center + math::Vec3{radius, radius, radius}};
    }
    case ecs::CombatVolumeComponent::Shape::Capsule: {
      const float radius = std::max(0.01f, volume.capsule.radius * std::max(scale.x, scale.z));
      const float halfHeight = std::max(0.01f, volume.capsule.height * 0.5f * scale.y);
      const math::Vec3 half{radius, halfHeight + radius, radius};
      return {center - half, center + half};
    }
    case ecs::CombatVolumeComponent::Shape::Box:
    default: {
      const math::Vec3 half{std::max(0.01f, volume.box.width * 0.5f * scale.x),
                            std::max(0.01f, volume.box.height * 0.5f * scale.y),
                            std::max(0.01f, volume.box.length * 0.5f * scale.z)};
      return {center - half, center + half};
    }
  }
}

void buildBoneWorldMatrices(const ecs::SkeletonComponent& skeleton,
                            std::vector<math::Mat4>& out,
                            std::vector<std::uint8_t>& computed) {
  const std::size_t boneCount = skeleton.bones.size();
  out.resize(boneCount);
  computed.assign(boneCount, 0u);

  const auto resolveBoneWorld = [&](auto&& self, int boneIndex) -> const math::Mat4& {
    const std::size_t idx = static_cast<std::size_t>(boneIndex);
    if (computed[idx] != 0u) return out[idx];

    const math::Mat4 local = skeleton.currentPose.size() > idx ? skeleton.currentPose[idx] : skeleton.bones[idx].localBindTransform;
    const int parent = skeleton.bones[idx].parentIndex;
    if (parent < 0 || parent >= static_cast<int>(boneCount) || skeleton.space == ecs::SkeletonComponent::Space::World) {
      out[idx] = local;
    } else {
      out[idx] = math::mul(self(self, parent), local);
    }
    computed[idx] = 1u;
    return out[idx];
  };

  for (std::size_t i = 0; i < boneCount; ++i) {
    (void)resolveBoneWorld(resolveBoneWorld, static_cast<int>(i));
  }
}

bool readFloatParam(const ecs::ShaderComponent& shader, const char* name, float& out) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<float>(&p.value)) {
      out = *v;
      found = true;
    }
  }
  return found;
}

bool readBaseColorParam(const ecs::ShaderComponent& shader, float& r, float& g, float& b) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != "baseColor") continue;
    if (const auto* c = std::get_if<render::Color>(&p.value)) {
      r = c->r;
      g = c->g;
      b = c->b;
      found = true;
    }
  }
  return found;
}

bool readColorParam(const ecs::ShaderComponent& shader, const char* name, float& r, float& g, float& b) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* c = std::get_if<render::Color>(&p.value)) {
      r = c->r;
      g = c->g;
      b = c->b;
      found = true;
    } else if (const auto* v = std::get_if<math::Vec3>(&p.value)) {
      r = v->x;
      g = v->y;
      b = v->z;
      found = true;
    }
  }
  return found;
}

bool readBoolParam(const ecs::ShaderComponent& shader, const char* name, bool& out) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<bool>(&p.value)) {
      out = *v;
      found = true;
    }
  }
  return found;
}

bool readIntParam(const ecs::ShaderComponent& shader, const char* name, int& out) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<int>(&p.value)) {
      out = *v;
      found = true;
    }
  }
  return found;
}

bool readVec2Param(const ecs::ShaderComponent& shader, const char* name, float& x, float& y) {
  bool found = false;
  for (const auto& p : shader.parameters) {
    if (p.name != name) continue;
    if (const auto* v = std::get_if<math::Vec2>(&p.value)) {
      x = v->x;
      y = v->y;
      found = true;
    }
  }
  return found;
}

const ecs::ShaderComponent::LodBreakpoint* selectLodBreakpoint(const ecs::ShaderComponent& shader, float distanceMeters) {
  const ecs::ShaderComponent::LodBreakpoint* chosen = nullptr;
  for (const auto& bp : shader.lodBreakpoints) {
    if (distanceMeters < bp.distanceMeters) continue;
    chosen = &bp;
  }
  return chosen;
}

ecs::ShaderComponent resolveShaderAtDistance(const ecs::ShaderComponent& shader, float distanceMeters) {
  ecs::ShaderComponent resolved = shader;
  const auto* bp = selectLodBreakpoint(shader, distanceMeters);
  if (bp) {
    resolved.textures.insert(resolved.textures.end(), bp->textures.begin(), bp->textures.end());
    resolved.parameters.insert(resolved.parameters.end(), bp->parameters.begin(), bp->parameters.end());
  }
  return resolved;
}

void applyBreakpointTessellation(const ecs::ShaderComponent& shader,
                                 float distanceMeters,
                                 float& tessNear,
                                 float& tessFar,
                                 float& tessMin,
                                 float& tessMax,
                                 int& tessQuality) {
  if (const auto* bp = selectLodBreakpoint(shader, distanceMeters); bp && bp->overrideTessellation) {
    tessNear = bp->tessNear;
    tessFar = bp->tessFar;
    tessMin = bp->tessMin;
    tessMax = bp->tessMax;
    tessQuality = bp->tessQuality;
  }
}

void extractKnownTextures(const ecs::ShaderComponent& shader, GraphicsSystem::TerrainDraw& out) {
  auto tryBind = [&](const char* slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        return;
      }
    }
  };

  tryBind("albedo", out.albedoTex, out.hasAlbedoTex);

  // Prefer OpenGL normal map if explicitly provided.
  tryBind("normalgl", out.normalTex, out.hasNormalTex);
  if (!out.hasNormalTex) {
    tryBind("normal", out.normalTex, out.hasNormalTex);
  }

  tryBind("roughness", out.roughnessTex, out.hasRoughnessTex);
  tryBind("ao", out.aoTex, out.hasAoTex);
  tryBind("ambient_occlusion", out.aoTex, out.hasAoTex);
  tryBind("displacement", out.displacementTex, out.hasDisplacementTex);

  // Per-terrain "tile maps" (aligned to terrain UVs).
  tryBind("height_map", out.heightMapTex, out.hasHeightMapTex);
  tryBind("terrain_normal_map", out.terrainNormalMapTex, out.hasTerrainNormalMapTex);
  tryBind("terrain_roughness_map", out.terrainRoughnessMapTex, out.hasTerrainRoughnessMapTex);
  tryBind("terrain_surface_map", out.terrainSurfaceMapTex, out.hasTerrainSurfaceMapTex);
  tryBind("splat_map", out.splatMapTex, out.hasSplatMapTex);
}

void extractRockLayerTextures(const ecs::ShaderComponent& shader, GraphicsSystem::TerrainDraw& out) {
  auto tryBind = [&](const char* slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        return;
      }
    }
  };

  tryBind("rock_albedo", out.rockAlbedoTex, out.hasRockAlbedoTex);
  tryBind("rock_normalgl", out.rockNormalTex, out.hasRockNormalTex);
  tryBind("rock_roughness", out.rockRoughnessTex, out.hasRockRoughnessTex);
  tryBind("rock_ao", out.rockAoTex, out.hasRockAoTex);
  tryBind("rock_displacement", out.rockDisplacementTex, out.hasRockDisplacementTex);
}

bool applySplatChannelRockOverrides(
    const ecs::ShaderComponent& shader,
    int splatChannel,
    GraphicsSystem::TerrainDraw& out) {
  const char channel =
      (splatChannel == 0) ? 'r' : (splatChannel == 1) ? 'g' : (splatChannel == 2) ? 'b' : (splatChannel == 3) ? 'a' : '\0';
  if (channel == '\0') return false;

  const std::string prefix = std::string("splat_") + channel + "_";
  bool any = false;

  auto tryBind = [&](const std::string& slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        any = true;
        return;
      }
    }
  };

  tryBind(prefix + "rock_albedo", out.rockAlbedoTex, out.hasRockAlbedoTex);
  tryBind(prefix + "rock_normalgl", out.rockNormalTex, out.hasRockNormalTex);
  tryBind(prefix + "rock_roughness", out.rockRoughnessTex, out.hasRockRoughnessTex);
  tryBind(prefix + "rock_ao", out.rockAoTex, out.hasRockAoTex);
  tryBind(prefix + "rock_displacement", out.rockDisplacementTex, out.hasRockDisplacementTex);

  bool channelRockEnabled = out.rockLayerEnabled;
  if (readBoolParam(shader, (prefix + "rockLayerEnabled").c_str(), channelRockEnabled)) {
    out.rockLayerEnabled = channelRockEnabled;
    any = true;
  } else if (any) {
    out.rockLayerEnabled = true;
  }

  (void)readVec2Param(shader, (prefix + "rockUvTiling").c_str(), out.rockUvTilingX, out.rockUvTilingY);
  (void)readFloatParam(shader, (prefix + "rockNormalScale").c_str(), out.rockNormalStrength);
  (void)readFloatParam(shader, (prefix + "rockDisplacementStrength").c_str(), out.rockDisplacementStrength);
  (void)readFloatParam(shader, (prefix + "rockBlendStrength").c_str(), out.rockBlendStrength);
  (void)readFloatParam(shader, (prefix + "rockNoiseScale").c_str(), out.rockNoiseScale);

  return any;
}

void extractGrassTextures(const ecs::ShaderComponent& shader, GraphicsSystem::FrameSnapshot::GrassDraw& out) {
  auto tryBind = [&](const char* slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        return;
      }
    }
  };

  out.grassTextures.clear();
  out.grassTextures.reserve(6);
  for (int i = 0; i < 6; ++i) {
    const std::string slot = std::string("grass_tex") + std::to_string(i);
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        out.grassTextures.push_back(t.texture);
        break;
      }
    }
  }

  // Back-compat: if no explicit list, fall back to the existing single-slot albedo.
  tryBind("grass_albedo", out.albedoTex, out.hasAlbedoTex);
  if (!out.hasAlbedoTex) tryBind("albedo", out.albedoTex, out.hasAlbedoTex);
  if (out.grassTextures.empty() && out.hasAlbedoTex) {
    out.grassTextures.push_back(out.albedoTex);
  }

  tryBind("density_mask", out.densityMaskTex, out.hasDensityMaskTex);
}

void extractMeshTextures(const ecs::ShaderComponent& shader, GraphicsSystem::MeshDraw& out) {
  auto tryBind = [&](const char* slot, render::AssetRef& dst, bool& has) {
    for (const auto& t : shader.textures) {
      if (t.slot == slot && t.texture.enabled && !t.texture.key.empty()) {
        dst = t.texture;
        has = true;
        return;
      }
    }
  };

  tryBind("albedo", out.albedoTex, out.hasAlbedoTex);
  tryBind("baseColor", out.albedoTex, out.hasAlbedoTex);
  tryBind("base_color", out.albedoTex, out.hasAlbedoTex);

  tryBind("normalgl", out.normalTex, out.hasNormalTex);
  if (!out.hasNormalTex) tryBind("normal", out.normalTex, out.hasNormalTex);

  tryBind("roughness", out.roughnessTex, out.hasRoughnessTex);
  tryBind("metallic", out.metallicTex, out.hasMetallicTex);
  tryBind("ao", out.aoTex, out.hasAoTex);
  if (!out.hasAoTex) tryBind("ambient_occlusion", out.aoTex, out.hasAoTex);
  tryBind("specular", out.specularTex, out.hasSpecularTex);
  tryBind("emissive", out.emissiveTex, out.hasEmissiveTex);
  tryBind("displacement", out.displacementTex, out.hasDisplacementTex);
  if (!out.hasDisplacementTex) tryBind("height", out.displacementTex, out.hasDisplacementTex);
  tryBind("metallicRoughness", out.metallicRoughnessTex, out.hasMetallicRoughnessTex);
  if (!out.hasMetallicRoughnessTex) tryBind("metallic_roughness", out.metallicRoughnessTex, out.hasMetallicRoughnessTex);
  tryBind("orm", out.ormTex, out.hasOrmTex);
}

}  // namespace

const GraphicsSystem::FrameSnapshot& GraphicsSystem::tick(EntityRegistry& registry) {
  m_frame.camera = {};
  m_frame.terrains.clear();
  m_frame.meshes.clear();
  m_frame.rays.clear();
  m_frame.debugLines.clear();
  m_frame.fogVolumes.clear();
  m_frame.skies.clear();
  m_frame.rocks.clear();
  m_frame.grasses.clear();
  m_frame.draw2d.clear();
  m_frame.billboards.clear();
  m_frame.vfx.clear();
  m_frame.lights.clear();
  m_frame.settings = {};

  // --- Camera (pick the first available) ---
  registry.view<ecs::CameraComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::CameraComponent& cam, const ecs::TransformComponent& tr) {
        if (m_frame.camera.entity != ecs::kInvalidEntityId) return;
        m_frame.camera.entity = id;
        m_frame.camera.position = tr.position;
        m_frame.camera.forward = forwardFromPitchYawDeg(tr);
        m_frame.camera.projectionType = static_cast<int>(cam.projectionType);
        m_frame.camera.fovYRadians = cam.fieldOfViewDeg * kDegToRad;
        m_frame.camera.orthographicSize = cam.orthographicSize;
        m_frame.camera.nearClip = cam.nearClipPlane;
        m_frame.camera.farClip = cam.farClipPlane;
        m_frame.camera.aspectRatio = cam.aspectRatio;
        m_frame.camera.useFramebufferAspectRatio = cam.useFramebufferAspectRatio;

        m_frame.camera.renderScale = cam.renderScale;

        m_frame.camera.depthOfFieldEnabled = cam.depthOfField.enabled;
        m_frame.camera.dofFocusDistance = cam.depthOfField.focusDistance;
        m_frame.camera.dofFocusRange = cam.depthOfField.focusRange;
        m_frame.camera.dofBlurStrength = cam.depthOfField.blurStrength;
        if (cam.depthOfField.enabled && cam.depthOfField.focusMode == ecs::CameraComponent::DepthOfFieldSettings::FocusMode::TargetEntity &&
            cam.depthOfField.focusTarget != ecs::kInvalidEntityId) {
          if (const auto* focusTr = registry.tryGet<ecs::TransformComponent>(cam.depthOfField.focusTarget)) {
            const math::Vec3 targetPos = focusTr->position + cam.depthOfField.focusTargetOffset;
            const float dist = math::length(targetPos - tr.position);
            m_frame.camera.dofFocusDistance = std::max(0.05f, dist);
          }
        }

        m_frame.camera.motionBlurEnabled = cam.motionBlur.enabled;
        m_frame.camera.motionBlurStrength = cam.motionBlur.strength;
        m_frame.camera.motionBlurMaxBlurPixels = cam.motionBlur.maxBlurPixels;
        m_frame.camera.motionBlurSamples = cam.motionBlur.samples;
      });

  // --- Render settings (pick the first enabled) ---
  registry.view<ecs::RenderSettingsComponent>([&](ecs::EntityId, const ecs::RenderSettingsComponent& s) {
    if (!s.enabled) return;
    if (m_frame.settings.present) return;
    m_frame.settings.present = true;
    m_frame.settings.shadowsEnabled = s.shadowsEnabled;
    m_frame.settings.shadowQuality = s.shadowQuality;
    m_frame.settings.shadowStrength = s.shadowStrength;
    m_frame.settings.shadowUseTessellation = s.shadowUseTessellation;
    m_frame.settings.showRays = s.showRays;
    m_frame.settings.showCollisionBoxes = s.showCollisionBoxes;
    m_frame.settings.showCombatBoxes = s.showCombatBoxes;
    m_frame.settings.showSkeletonBones = s.showSkeletonBones;
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
        draw.noiseSeed = terrain.noiseSeed;
        draw.lodMaxRenderDistance = terrain.lodMaxRenderDistance;
        draw.lodStep1Distance = terrain.lodStep1Distance;
        draw.lodStep2Distance = terrain.lodStep2Distance;
        draw.lodStep4Distance = terrain.lodStep4Distance;
        draw.lodStep8Distance = terrain.lodStep8Distance;
        draw.lodStep16Distance = terrain.lodStep16Distance;
        draw.lodForceNearDistance = terrain.lodForceNearDistance;
        draw.tessLockDistance = terrain.tessLockDistance;
        draw.tessEnableDistance = terrain.tessEnableDistance;
        draw.tessDisableDistance = terrain.tessDisableDistance;
        draw.viewDotBias = terrain.viewDotBias;
        const math::Vec3 cameraPos = m_frame.camera.position;
        const float terrainDistance = [&]() {
          const float sizeX = static_cast<float>(std::max(2, terrain.gridWidth)) * terrain.cellSizeMeters;
          const float sizeZ = static_cast<float>(std::max(2, terrain.gridHeight)) * terrain.cellSizeMeters;
          const float halfX = 0.5f * sizeX;
          const float halfZ = 0.5f * sizeZ;
          const float nearestX = std::max(draw.position.x - halfX, std::min(cameraPos.x, draw.position.x + halfX));
          const float nearestZ = std::max(draw.position.z - halfZ, std::min(cameraPos.z, draw.position.z + halfZ));
          const float dx = nearestX - cameraPos.x;
          const float dz = nearestZ - cameraPos.z;
          return std::sqrt(dx * dx + dz * dz);
        }();
        ecs::ShaderComponent resolvedShader = resolveShaderAtDistance(shader, terrainDistance);

        draw.shader = shader.shader;
        draw.renderMode = static_cast<int>(shader.renderMode);
        draw.cullMode = static_cast<int>(shader.cullMode);
        draw.depthTest = static_cast<int>(shader.depthTest);
        draw.blendMode = static_cast<int>(shader.blendMode);
        draw.depthWrite = shader.depthWrite;
        draw.doubleSided = shader.doubleSided;
        draw.receiveShadows = shader.receiveShadows;
        draw.castShadows = shader.castShadows;
        draw.textureCount = resolvedShader.textures.size();
        draw.parameterCount = resolvedShader.parameters.size();

        (void)readBaseColorParam(resolvedShader, draw.baseColorR, draw.baseColorG, draw.baseColorB);
        (void)readFloatParam(resolvedShader, "roughness", draw.roughness);
        (void)readBoolParam(resolvedShader, "roughnessInvert", draw.roughnessInvert);
        (void)readFloatParam(resolvedShader, "metallic", draw.metallic);
        (void)readFloatParam(resolvedShader, "specularIntensity", draw.specularIntensity);
        (void)readFloatParam(resolvedShader, "dirtColorNoiseStrength", draw.dirtColorNoiseStrength);

        extractKnownTextures(resolvedShader, draw);
        (void)readVec2Param(resolvedShader, "uvTiling", draw.uvTilingX, draw.uvTilingY);
        (void)readFloatParam(resolvedShader, "normalScale", draw.normalStrength);
        (void)readFloatParam(resolvedShader, "aoStrength", draw.aoStrength);
        (void)readFloatParam(resolvedShader, "displacementStrength", draw.displacementStrength);
        (void)readBoolParam(resolvedShader, "displacementInvert", draw.displacementInvert);

        // Tile map controls (optional; used for height/normal/roughness/surface/splat maps).
        (void)readVec2Param(resolvedShader, "mapUvTiling", draw.mapUvTilingX, draw.mapUvTilingY);
        (void)readFloatParam(resolvedShader, "heightMapStrength", draw.heightMapStrength);
        (void)readBoolParam(resolvedShader, "heightMapInvert", draw.heightMapInvert);
        (void)readFloatParam(resolvedShader, "heightMapMipBias", draw.heightMapMipBias);
        (void)readIntParam(resolvedShader, "heightMapQuality", draw.heightMapQuality);
        (void)readFloatParam(resolvedShader, "terrainNormalMapStrength", draw.terrainNormalMapStrength);
        (void)readFloatParam(resolvedShader, "terrainRoughnessMapStrength", draw.terrainRoughnessMapStrength);
        (void)readBoolParam(resolvedShader, "terrainRoughnessInvert", draw.terrainRoughnessInvert);
        (void)readFloatParam(resolvedShader, "terrainSurfaceStrength", draw.terrainSurfaceStrength);
        (void)readFloatParam(resolvedShader, "splatStrength", draw.splatStrength);
        (void)readIntParam(resolvedShader, "splatChannel", draw.splatChannel);
        (void)readFloatParam(resolvedShader, "mapMipBias", draw.mapMipBias);
        (void)readFloatParam(resolvedShader, "mapMipScale", draw.mapMipScale);
        (void)readFloatParam(resolvedShader, "mapMipMax", draw.mapMipMax);

        // Tessellation controls (if present on the material).
        (void)readFloatParam(resolvedShader, "tessNear", draw.tessNear);
        (void)readFloatParam(resolvedShader, "tessFar", draw.tessFar);
        (void)readFloatParam(resolvedShader, "tessMin", draw.tessMin);
        (void)readFloatParam(resolvedShader, "tessMax", draw.tessMax);
        (void)readIntParam(resolvedShader, "tessQuality", draw.tessQuality);
        applyBreakpointTessellation(shader, terrainDistance, draw.tessNear, draw.tessFar, draw.tessMin, draw.tessMax, draw.tessQuality);

        (void)readBoolParam(resolvedShader, "rockLayerEnabled", draw.rockLayerEnabled);
        extractRockLayerTextures(resolvedShader, draw);
        (void)readVec2Param(resolvedShader, "rockUvTiling", draw.rockUvTilingX, draw.rockUvTilingY);
        (void)readFloatParam(resolvedShader, "rockNormalScale", draw.rockNormalStrength);
        (void)readFloatParam(resolvedShader, "rockDisplacementStrength", draw.rockDisplacementStrength);
        (void)readFloatParam(resolvedShader, "rockBlendStrength", draw.rockBlendStrength);
        (void)readFloatParam(resolvedShader, "rockNoiseScale", draw.rockNoiseScale);
        (void)applySplatChannelRockOverrides(resolvedShader, draw.splatChannel, draw);
        m_frame.terrains.push_back(std::move(draw));
      });

  // --- Mesh renderables ---
  registry.view<ecs::MeshComponent, ecs::ShaderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::MeshComponent& mesh, const ecs::ShaderComponent& shader, const ecs::TransformComponent& tr) {
        if (!mesh.enabled || !mesh.visible || !shader.enabled) return;
        MeshDraw draw;
        draw.entity = id;
        draw.position = tr.position;
        draw.rotation = tr.rotation;
        draw.scale = {tr.scale.x * mesh.scale.x, tr.scale.y * mesh.scale.y, tr.scale.z * mesh.scale.z};
        auto& transformCache = m_meshTransformCache[id];
        if (!transformCache.valid || !sameVec3(transformCache.position, draw.position) ||
            !sameVec3(transformCache.rotation, draw.rotation) || !sameVec3(transformCache.scale, draw.scale)) {
          transformCache.position = draw.position;
          transformCache.rotation = draw.rotation;
          transformCache.scale = draw.scale;
          transformCache.modelMatrix = composeTransform(draw.position, draw.rotation, draw.scale);
          transformCache.valid = true;
        }
        draw.modelMatrix = transformCache.modelMatrix;
        draw.meshData = mesh.meshData;
        draw.shader = shader.shader;
        draw.visible = mesh.visible;
        draw.castShadows = mesh.castShadows && shader.castShadows;
        draw.receiveShadows = mesh.receiveShadows && shader.receiveShadows;
        const float meshDistance = math::length(m_frame.camera.position - draw.position);
        ecs::ShaderComponent resolvedShader = resolveShaderAtDistance(shader, meshDistance);

        draw.hasBaseColorParam = readBaseColorParam(resolvedShader, draw.baseColorR, draw.baseColorG, draw.baseColorB);
        draw.hasRoughnessParam = readFloatParam(resolvedShader, "roughness", draw.roughness);
        draw.hasMetallicParam = readFloatParam(resolvedShader, "metallic", draw.metallic);
        draw.hasSpecularIntensityParam = readFloatParam(resolvedShader, "specularIntensity", draw.specularIntensity);
        draw.hasNormalStrengthParam = readFloatParam(resolvedShader, "normalScale", draw.normalStrength);
        if (!draw.hasNormalStrengthParam) {
          draw.hasNormalStrengthParam = readFloatParam(resolvedShader, "normalStrength", draw.normalStrength);
        }
        draw.hasAoStrengthParam = readFloatParam(resolvedShader, "aoStrength", draw.aoStrength);
        draw.hasEmissiveStrengthParam = readFloatParam(resolvedShader, "emissiveStrength", draw.emissiveStrength);
        draw.hasDisplacementStrengthParam = readFloatParam(resolvedShader, "displacementStrength", draw.displacementStrength);
        draw.hasEmissiveColorParam =
            readColorParam(resolvedShader, "emissiveColor", draw.emissiveColorR, draw.emissiveColorG, draw.emissiveColorB);
        extractMeshTextures(resolvedShader, draw);
        (void)readFloatParam(resolvedShader, "tessNear", draw.tessNear);
        (void)readFloatParam(resolvedShader, "tessFar", draw.tessFar);
        (void)readFloatParam(resolvedShader, "tessMin", draw.tessMin);
        (void)readFloatParam(resolvedShader, "tessMax", draw.tessMax);
        (void)readIntParam(resolvedShader, "tessQuality", draw.tessQuality);
        applyBreakpointTessellation(shader, meshDistance, draw.tessNear, draw.tessFar, draw.tessMin, draw.tessMax, draw.tessQuality);
        if (const auto* skeleton = registry.tryGet<ecs::SkeletonComponent>(id)) {
          const std::size_t count = std::min<std::size_t>(96, std::min(skeleton->bones.size(), skeleton->inverseBindMatrices.size()));
          if (skeleton->enabled && count > 0) {
            buildBoneWorldMatrices(*skeleton, m_boneWorldScratch, m_boneWorldComputedScratch);
            draw.hasSkinning = true;
            draw.skinMatrixCount = count;
            for (std::size_t i = 0; i < count; ++i) {
              draw.skinMatrices[i] = math::mul(m_boneWorldScratch[i], skeleton->inverseBindMatrices[i]);
            }
          }
        }
        m_frame.meshes.push_back(std::move(draw));
      });

  // --- Ray debug lines ---
  registry.view<ecs::RaycastComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::RaycastComponent& ray, const ecs::TransformComponent& tr) {
        if (!m_frame.settings.showRays || !ray.enabled || !ray.debugDraw) return;

        math::Vec3 origin{};
        if (ray.originMode == ecs::RaycastComponent::OriginMode::WorldPosition) {
          if (!ray.hasWorldPosition) return;
          origin = ray.worldPosition;
        } else {
          origin = tr.position + ray.localOffset;
        }

        math::Vec3 direction{0.0f, 0.0f, 1.0f};
        switch (ray.directionMode) {
          case ecs::RaycastComponent::DirectionMode::Forward:
            direction = forwardFromRotationDeg(tr.rotation);
            break;
          case ecs::RaycastComponent::DirectionMode::Up:
            direction = rotateVector({0.0f, 1.0f, 0.0f}, tr.rotation);
            break;
          case ecs::RaycastComponent::DirectionMode::Down:
            direction = rotateVector({0.0f, -1.0f, 0.0f}, tr.rotation);
            break;
          case ecs::RaycastComponent::DirectionMode::Right:
            direction = rotateVector({1.0f, 0.0f, 0.0f}, tr.rotation);
            break;
          case ecs::RaycastComponent::DirectionMode::Left:
            direction = rotateVector({-1.0f, 0.0f, 0.0f}, tr.rotation);
            break;
          case ecs::RaycastComponent::DirectionMode::TowardTarget: {
            const auto* targetTr = registry.tryGet<ecs::TransformComponent>(ray.towardTargetEntity);
            if (!targetTr) return;
            direction = math::normalize((targetTr->position + ray.towardTargetOffset) - origin);
            break;
          }
          case ecs::RaycastComponent::DirectionMode::CustomVector:
          default:
            direction = rotateVector(ray.customDirection, tr.rotation);
            break;
        }

        direction = math::normalize(direction);
        if (math::lengthSq(direction) <= 0.0f) return;

        ecs::systems::GraphicsSystem::FrameSnapshot::RayDraw draw;
        draw.entity = id;
        draw.sensorEntity = ray.sensorEntity;
        draw.start = origin;
        draw.hit = !ray.hitResults.empty();
        draw.end = draw.hit ? ray.hitResults.front().hitPosition : (origin + direction * ray.length);
        draw.color = draw.hit ? render::Color{1.0f, 0.15f, 0.15f, 1.0f}
                               : render::Color{ray.debugColor.r, ray.debugColor.g, ray.debugColor.b, ray.debugColor.a};
        draw.category = ray.raycastCategory;
        m_frame.rays.push_back(std::move(draw));
      });

  // --- Debug volume boxes and skeleton bones ---
  if (m_frame.settings.showCollisionBoxes || m_frame.settings.showCombatBoxes || m_frame.settings.showSkeletonBones) {
    if (m_frame.settings.showCollisionBoxes) {
      registry.view<ecs::ColliderComponent, ecs::TransformComponent>(
          [&](ecs::EntityId, const ecs::ColliderComponent& collider, const ecs::TransformComponent& tr) {
            const auto bounds = colliderAabb(tr, collider);
            addAabbLines(m_frame.debugLines, bounds, {1.0f, 1.0f, 0.0f, 1.0f});
          });
    }

    if (m_frame.settings.showCombatBoxes) {
      registry.view<ecs::CombatVolumeComponent, ecs::TransformComponent>(
          [&](ecs::EntityId, const ecs::CombatVolumeComponent& combat, const ecs::TransformComponent& tr) {
            for (const auto& volume : combat.volumes) {
              const auto bounds = combatAabb(tr, volume);
              const float alpha = volume.enabled ? 1.0f : 0.25f;
              const render::Color color = volume.role == ecs::CombatVolumeComponent::Role::Hit
                                              ? render::Color{1.0f, 0.0f, 0.0f, alpha}
                                              : render::Color{1.0f, 0.5f, 0.0f, alpha};
              addAabbLines(m_frame.debugLines, bounds, color);
            }
          });
    }

    if (m_frame.settings.showSkeletonBones) {
      registry.view<ecs::SkeletonComponent, ecs::TransformComponent>(
          [&](ecs::EntityId id, const ecs::SkeletonComponent& skeleton, const ecs::TransformComponent& tr) {
            if (!skeleton.enabled || skeleton.bones.empty()) return;
            buildBoneWorldMatrices(skeleton, m_boneWorldScratch, m_boneWorldComputedScratch);
            math::Mat4 rootWorld = composeWorld(tr);
            if (const auto* mesh = registry.tryGet<ecs::MeshComponent>(id)) {
              rootWorld = math::mul(rootWorld, math::scale(mesh->scale));
            }
            for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
              const int parent = skeleton.bones[i].parentIndex;
              if (parent < 0) continue;
              const math::Vec3 childPos = translationFromMat4(math::mul(rootWorld, m_boneWorldScratch[i]));
              const math::Vec3 parentPos = translationFromMat4(math::mul(rootWorld, m_boneWorldScratch[static_cast<std::size_t>(parent)]));
              addLine(m_frame.debugLines, parentPos, childPos, {1.0f, 1.0f, 1.0f, 1.0f});
            }
          });
    }
  }

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
        draw.shadowResolution = light.shadowResolution;
        draw.shadowBias = light.shadowBias;
        draw.shadowDistance = light.shadowDistance;
        m_frame.lights.push_back(std::move(draw));
      });

  // --- Fog volumes (pick the first enabled) ---
  registry.view<ecs::FogVolumeComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::FogVolumeComponent& fog, const ecs::TransformComponent& tr) {
        if (!fog.enabled) return;
        if (!m_frame.fogVolumes.empty()) return;
        FrameSnapshot::FogDraw draw;
        draw.entity = id;
        draw.center = tr.position;
        draw.sizeMeters = fog.sizeMeters;
        draw.color = fog.color;
        draw.density = fog.density;
        draw.startDistance = fog.startDistance;
        draw.endDistance = fog.endDistance;
        draw.heightFalloff = fog.heightFalloff;
        draw.baseHeightOffset = fog.baseHeightOffset;
        m_frame.fogVolumes.push_back(std::move(draw));
      });

  // --- Skies (pick the first enabled) ---
  registry.view<ecs::SkyComponent, ecs::ShaderComponent>(
      [&](ecs::EntityId id, const ecs::SkyComponent& sky, const ecs::ShaderComponent& shader) {
        if (!sky.enabled) return;
        if (!shader.enabled) return;
        if (!m_frame.skies.empty()) return;
        FrameSnapshot::SkyDraw draw;
        draw.entity = id;
        draw.shader = shader.shader;
        draw.horizonColor = sky.horizonColor;
        draw.zenithColor = sky.zenithColor;
        draw.sunEnabled = sky.sunEnabled;
        draw.sunDirection = sky.sunDirection;
        draw.sunTint = sky.sunTint;
        draw.sunDiscIntensity = sky.sunDiscIntensity;
        draw.sunDiscSize = sky.sunDiscSize;
        draw.cloudsEnabled = sky.cloudsEnabled;
        draw.skyType = static_cast<int>(sky.skyType);
        draw.cloudType = static_cast<int>(sky.cloudType);
        draw.quality = static_cast<int>(sky.quality);
        draw.cloudCoverage = sky.cloudCoverage;
        draw.cloudDensity = sky.cloudDensity;
        draw.cloudSpeed = sky.cloudSpeed;
        draw.cloudWindX = sky.cloudWindDirection.x;
        draw.cloudWindZ = sky.cloudWindDirection.y;
        draw.cloudTimeScale = sky.cloudTimeScale;
        draw.cloudTurbulence = sky.cloudTurbulence;
        draw.cloudScale = sky.cloudScale;
        draw.cloudLightAbsorption = sky.cloudLightAbsorption;
        draw.cloudHeightMeters = sky.cloudHeightMeters;

        draw.starsEnabled = sky.starsEnabled;
        draw.starsIntensity = sky.starsIntensity;
        draw.starsDensity = sky.starsDensity;
        draw.starsSize = sky.starsSize;
        draw.starsTwinkleStrength = sky.starsTwinkleStrength;
        draw.starsTwinkleSpeed = sky.starsTwinkleSpeed;
        draw.starsSeed = sky.starsSeed;
        m_frame.skies.push_back(std::move(draw));
      });

  // --- Rock scatters with shaders ---
  registry.view<ecs::RockScatterComponent, ecs::ShaderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::RockScatterComponent& rocks,
          const ecs::ShaderComponent& shader,
          const ecs::TransformComponent& tr) {
        if (!rocks.enabled) return;
        if (!shader.enabled) return;
        FrameSnapshot::RockDraw draw;
        draw.entity = id;
        draw.position = tr.position;
        draw.area = rocks.area;
        draw.density = rocks.density;
        draw.seed = rocks.seed;
        draw.minScale = rocks.minScale;
        draw.maxScale = rocks.maxScale;
        draw.clumpiness = rocks.clumpiness;
        draw.patchScale = rocks.patchScale;
        draw.lodBias = rocks.lodBias;
        draw.castShadows = rocks.castShadows;
        draw.receiveShadows = rocks.receiveShadows;
        draw.shader = shader.shader;
        m_frame.rocks.push_back(std::move(draw));
      });

  // --- Grass patches with shaders ---
  registry.view<ecs::GrassPatchComponent, ecs::ShaderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::GrassPatchComponent& grass,
          const ecs::ShaderComponent& shader,
          const ecs::TransformComponent& tr) {
        if (!grass.enabled) return;
        if (!shader.enabled) return;
        if (grass.layers.empty()) return;

        FrameSnapshot::GrassDraw draw;
        draw.entity = id;
        draw.sourceTerrainEntity = grass.sourceTerrainEntity;
        draw.position = tr.position;
        draw.area = grass.area;
        draw.densityMultiplier = grass.densityMultiplier;
        draw.seed = grass.seed;
        draw.densityNoise = grass.densityNoise;
        draw.densityNoiseThreshold = grass.densityNoiseThreshold;
        draw.densityNoiseContrast = grass.densityNoiseContrast;
        draw.densityNoiseStrength = grass.densityNoiseStrength;
        draw.islandNoise = grass.islandNoise;
        draw.islandNoiseOffset = grass.islandNoiseOffset;
        draw.islandNoiseThreshold = grass.islandNoiseThreshold;
        draw.islandNoiseSoftness = grass.islandNoiseSoftness;
        draw.islandNoiseContrast = grass.islandNoiseContrast;
        draw.islandNoiseStrength = grass.islandNoiseStrength;
        draw.castShadows = grass.castShadows;
        draw.receiveShadows = grass.receiveShadows;
        draw.lodBias = grass.lodBias;
        draw.interactionEnabled = grass.interactionEnabled;
        draw.interactionRadiusMeters = grass.interactionRadiusMeters;
        draw.interactionStrength = grass.interactionStrength;
        draw.shader = shader.shader;

        const float grassDistance = math::length(m_frame.camera.position - draw.position);
        ecs::ShaderComponent resolvedShader = resolveShaderAtDistance(shader, grassDistance);

        extractGrassTextures(resolvedShader, draw);
        (void)readFloatParam(resolvedShader, "grassAlbedoUvScale", draw.albedoUvScale);
        (void)readFloatParam(resolvedShader, "densityMaskStrength", draw.densityMaskStrength);
        (void)readFloatParam(resolvedShader, "densityMaskTiling", draw.densityMaskTiling);
        (void)readBoolParam(resolvedShader, "densityMaskInvert", draw.densityMaskInvert);
        (void)readFloatParam(resolvedShader, "densityMaskScaleStrength", draw.densityMaskScaleStrength);
        (void)readFloatParam(resolvedShader, "densityMaskScalePower", draw.densityMaskScalePower);

        draw.layers.reserve(grass.layers.size());
        for (const auto& l : grass.layers) {
          FrameSnapshot::GrassLayerDraw ld;
          ld.species = l.species;
          ld.density = l.density;
          ld.minScale = l.minScale;
          ld.maxScale = l.maxScale;
          ld.bladeSpacing = l.bladeSpacing;
          ld.bendStrength = l.bendStrength;
          ld.curveStrength = l.curveStrength;
          ld.twistStrength = l.twistStrength;
          ld.minSlopeDeg = l.minSlopeDeg;
          ld.maxSlopeDeg = l.maxSlopeDeg;
          ld.minAltitude = l.minAltitude;
          ld.maxAltitude = l.maxAltitude;
          ld.noiseScale = l.noiseScale;
          ld.noiseStrength = l.noiseStrength;
          ld.windStrength = l.windStrength;
          ld.maxDistance = l.maxDistance;
          draw.layers.push_back(std::move(ld));
        }

        m_frame.grasses.push_back(std::move(draw));
      });

  // --- VFX emitters ---
  registry.view<ecs::VfxComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::VfxComponent& vfx, const ecs::TransformComponent& tr) {
        if (!vfx.enabled) return;
        FrameSnapshot::VfxDraw draw;
        draw.entity = id;
        draw.position = tr.position;
        draw.rotation = tr.rotation;
        draw.scale = tr.scale;
        draw.type = vfx.type;
        draw.quality = vfx.quality;
        draw.enabled = vfx.enabled;
        draw.autoQuality = vfx.autoQuality;
        draw.maxRenderDistance = vfx.maxRenderDistance;
        draw.lodNearDistance = vfx.lodNearDistance;
        draw.lodMidDistance = vfx.lodMidDistance;
        draw.lodFarDistance = vfx.lodFarDistance;
        draw.lodUltraDistance = vfx.lodUltraDistance;
        draw.lodForceNearDistance = vfx.lodForceNearDistance;
        draw.viewDotBias = vfx.viewDotBias;
        draw.intensity = vfx.intensity;
        draw.spawnRate = vfx.spawnRate;
        draw.burstInterval = vfx.burstInterval;
        draw.lifetimeSeconds = vfx.lifetimeSeconds;
        draw.sizeMeters = vfx.sizeMeters;
        draw.sizeVariance = vfx.sizeVariance;
        draw.speedMetersPerSecond = vfx.speedMetersPerSecond;
        draw.speedVariance = vfx.speedVariance;
        draw.gravityScale = vfx.gravityScale;
        draw.drag = vfx.drag;
        draw.flickerStrength = vfx.flickerStrength;
        draw.flickerSpeed = vfx.flickerSpeed;
        draw.looping = vfx.looping;
        draw.castLight = vfx.castLight;
        draw.seed = vfx.seed;
        draw.heightMeters = vfx.heightMeters;
        draw.upwardBias = vfx.upwardBias;
        draw.spreadRadiusMeters = vfx.spreadRadiusMeters;
        draw.heatHazeStrength = vfx.heatHazeStrength;
        draw.chargeLengthMeters = vfx.chargeLengthMeters;
        draw.arcJitter = vfx.arcJitter;
        draw.branchCount = vfx.branchCount;
        draw.segmentCount = vfx.segmentCount;
        draw.pulseSpeed = vfx.pulseSpeed;
        draw.sparkCount = vfx.sparkCount;
        draw.sparkSpreadDegrees = vfx.sparkSpreadDegrees;
        draw.sparkTrailLengthMeters = vfx.sparkTrailLengthMeters;
        draw.sparkFadeSeconds = vfx.sparkFadeSeconds;
        draw.primaryColor = vfx.primaryColor;
        draw.secondaryColor = vfx.secondaryColor;
        m_frame.vfx.push_back(std::move(draw));
      });

  // --- Screen-space 2D quads ---
  registry.view<ecs::Draw2DComponent>([&](ecs::EntityId id, const ecs::Draw2DComponent& draw2d) {
    if (!draw2d.enabled) return;
    for (const auto& q : draw2d.quads) {
      if (!q.enabled) continue;
      FrameSnapshot::Draw2DQuadDraw draw;
      draw.entity = id;
      draw.name = q.name;
      draw.enabled = q.enabled;
      draw.layer = q.layer;
      draw.anchor = q.anchor;
      draw.offsetPx = q.offsetPx;
      draw.sizePx = q.sizePx;
      draw.textureEnabled = q.textureEnabled;
      draw.texture = q.texture;
      draw.uv0 = q.uv0;
      draw.uv1 = q.uv1;
      draw.colorTL = q.colorTL;
      draw.colorTR = q.colorTR;
      draw.colorBR = q.colorBR;
      draw.colorBL = q.colorBL;
      m_frame.draw2d.push_back(std::move(draw));
    }
  });

  // --- World billboards ---
  registry.view<ecs::BillboardComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::BillboardComponent& billboard, const ecs::TransformComponent& tr) {
        if (!billboard.enabled || !billboard.visible) return;
        FrameSnapshot::BillboardDraw draw;
        draw.entity = id;
        draw.enabled = billboard.enabled;
        draw.visible = billboard.visible;
        draw.textureEnabled = billboard.textureEnabled;
        draw.depthWrite = billboard.depthWrite;
        draw.doubleSided = billboard.doubleSided;
        draw.faceMode = billboard.faceMode;
        draw.position = tr.position + billboard.worldOffset;
        draw.rotation = tr.rotation + billboard.rotationOffsetDeg;
        draw.sizeMeters = billboard.sizeMeters;
        draw.pivot = billboard.pivot;
        draw.maxRenderDistance = billboard.maxRenderDistance;
        draw.texture = billboard.texture;
        draw.animatedTexture = billboard.animatedTexture;
        draw.tint = billboard.tint;
        m_frame.billboards.push_back(std::move(draw));
      });

  return m_frame;
}

}  // namespace ecs::systems
