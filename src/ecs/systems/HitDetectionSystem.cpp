#include "ecs/systems/HitDetectionSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/CombatVolumeComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/CombatEvents.h"
#include "math/Vec3.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace {

using Aabb = ecs::services::SpatialHashGridService::Aabb;

math::Vec3 rotateVector(const math::Vec3& v, const math::Vec3& rotationDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  const float rx = rotationDeg.x * kPi / 180.0f;
  const float ry = rotationDeg.y * kPi / 180.0f;
  const float rz = rotationDeg.z * kPi / 180.0f;

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

struct CombatInstance final {
  ecs::EntityId entity = ecs::kInvalidEntityId;
  int volumeIndex = -1;
  ecs::CombatVolumeComponent::Volume volume{};
  Aabb bounds{};
  ecs::TransformComponent transform{};
};

std::uint64_t contactKey(const CombatInstance& hit, const CombatInstance& hurt) {
  const std::uint64_t hitEntity = static_cast<std::uint64_t>(hit.entity);
  const std::uint64_t hurtEntity = static_cast<std::uint64_t>(hurt.entity);
  const std::uint64_t hitVolume = static_cast<std::uint64_t>(static_cast<std::uint32_t>(hit.volumeIndex));
  const std::uint64_t hurtVolume = static_cast<std::uint64_t>(static_cast<std::uint32_t>(hurt.volumeIndex));
  return (hitEntity << 32u) ^ (hurtEntity << 1u) ^ (hitVolume << 16u) ^ hurtVolume;
}

Aabb makeBounds(const ecs::TransformComponent& tr, const ecs::CombatVolumeComponent::Volume& volume) {
  const math::Vec3 localOffset = rotateVector(volume.offset, tr.rotation);
  const math::Vec3 center = tr.position + localOffset;
  const math::Vec3 scale{std::max(0.01f, tr.scale.x), std::max(0.01f, tr.scale.y), std::max(0.01f, tr.scale.z)};

  switch (volume.shape) {
    case ecs::CombatVolumeComponent::Shape::Sphere: {
      const float radius = std::max(0.01f, volume.sphere.radius * std::max({scale.x, scale.y, scale.z}));
      const math::Vec3 half{radius, radius, radius};
      return {center - half, center + half};
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

bool intersects(const Aabb& a, const Aabb& b) {
  return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y && a.min.z <= b.max.z &&
         a.max.z >= b.min.z;
}

ecs::events::CombatContactDetectEvent makeContact(const CombatInstance& hit,
                                                  const CombatInstance& hurt,
                                                  double timeSeconds) {
  const math::Vec3 hitCenter = (hit.bounds.min + hit.bounds.max) * 0.5f;
  const math::Vec3 hurtCenter = (hurt.bounds.min + hurt.bounds.max) * 0.5f;
  const float ox = std::min(hit.bounds.max.x - hurt.bounds.min.x, hurt.bounds.max.x - hit.bounds.min.x);
  const float oy = std::min(hit.bounds.max.y - hurt.bounds.min.y, hurt.bounds.max.y - hit.bounds.min.y);
  const float oz = std::min(hit.bounds.max.z - hurt.bounds.min.z, hurt.bounds.max.z - hit.bounds.min.z);

  math::Vec3 normal{(hitCenter.x >= hurtCenter.x) ? 1.0f : -1.0f, 0.0f, 0.0f};
  float depth = ox;
  if (oy < depth) {
    depth = oy;
    normal = {0.0f, (hitCenter.y >= hurtCenter.y) ? 1.0f : -1.0f, 0.0f};
  }
  if (oz < depth) {
    depth = oz;
    normal = {0.0f, 0.0f, (hitCenter.z >= hurtCenter.z) ? 1.0f : -1.0f};
  }

  ecs::events::CombatContactDetectEvent e;
  e.hitEntity = hit.entity;
  e.hurtEntity = hurt.entity;
  e.hitVolumeIndex = hit.volumeIndex;
  e.hurtVolumeIndex = hurt.volumeIndex;
  e.contactPoint = (hitCenter + hurtCenter) * 0.5f;
  e.contactNormal = normal;
  e.penetrationDepth = std::max(0.0f, depth);
  e.damage = hit.volume.damage * hurt.volume.damageMultiplier;
  e.force = hit.volume.force;
  e.damageType = hit.volume.damageType;
  e.time = timeSeconds;
  return e;
}

}  // namespace

namespace ecs::systems {

void HitDetectionSystem::tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) {
  std::vector<CombatInstance> instances;
  instances.reserve(64);
  m_grid.clear();
  std::unordered_set<std::uint64_t> activeContacts;

  registry.view<ecs::CombatVolumeComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::CombatVolumeComponent& combat, const ecs::TransformComponent& tr) {
        if (combat.volumes.empty()) return;
        for (std::size_t i = 0; i < combat.volumes.size(); ++i) {
          const auto& volume = combat.volumes[i];
          if (!volume.enabled) continue;

          CombatInstance instance;
          instance.entity = id;
          instance.volumeIndex = static_cast<int>(i);
          instance.volume = volume;
          instance.transform = tr;
          instance.bounds = makeBounds(tr, volume);
          instances.push_back(instance);
          m_grid.insert({static_cast<ecs::EntityId>(instances.size()), instance.bounds});
        }
      });

  auto instanceForProxy = [&](ecs::EntityId proxyId) -> const CombatInstance* {
    if (proxyId == ecs::kInvalidEntityId) return nullptr;
    const std::size_t idx = static_cast<std::size_t>(proxyId - 1);
    if (idx >= instances.size()) return nullptr;
    return &instances[idx];
  };

  for (const auto& [aProxy, bProxy] : m_grid.candidatePairs()) {
    const CombatInstance* a = instanceForProxy(aProxy);
    const CombatInstance* b = instanceForProxy(bProxy);
    if (!a || !b) continue;
    if (a->entity == b->entity) continue;
    if (!intersects(a->bounds, b->bounds)) continue;

    const bool aIsHit = a->volume.role == ecs::CombatVolumeComponent::Role::Hit;
    const bool bIsHit = b->volume.role == ecs::CombatVolumeComponent::Role::Hit;
    const bool aIsHurt = a->volume.role == ecs::CombatVolumeComponent::Role::Hurt;
    const bool bIsHurt = b->volume.role == ecs::CombatVolumeComponent::Role::Hurt;
    if ((aIsHit && bIsHurt)) {
      const auto key = contactKey(*a, *b);
      activeContacts.insert(key);
      if (m_activeContacts.find(key) == m_activeContacts.end()) {
        events.emit<ecs::events::CombatContactDetectEvent>(makeContact(*a, *b, timeSeconds));
      }
    } else if (bIsHit && aIsHurt) {
      const auto key = contactKey(*b, *a);
      activeContacts.insert(key);
      if (m_activeContacts.find(key) == m_activeContacts.end()) {
        events.emit<ecs::events::CombatContactDetectEvent>(makeContact(*b, *a, timeSeconds));
      }
    }
  }

  m_activeContacts.swap(activeContacts);
}

}  // namespace ecs::systems
