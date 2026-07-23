#include "ecs/systems/RayDetectionSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/SensorComponent.h"
#include "ecs/components/RaycastComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/RaycastEvents.h"
#include "math/Vec3.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace {

using Aabb = ecs::services::SpatialHashGridService::Aabb;

Aabb colliderAabb(const ecs::TransformComponent& tr, const ecs::ColliderComponent& c) {
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

Aabb expanded(const Aabb& box, float radius) {
  const math::Vec3 pad{radius, radius, radius};
  return {box.min - pad, box.max + pad};
}

math::Vec3 eulerToForward(const math::Vec3& rotationDeg) {
  const float pitch = rotationDeg.x * 3.14159265358979323846f / 180.0f;
  const float yaw = rotationDeg.y * 3.14159265358979323846f / 180.0f;
  const float cp = std::cos(pitch);
  const float sp = std::sin(pitch);
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  return math::normalize({sy * cp, -sp, cy * cp});
}

math::Vec3 rotateVector(const math::Vec3& v, const math::Vec3& rotationDeg) {
  const float rx = rotationDeg.x * 3.14159265358979323846f / 180.0f;
  const float ry = rotationDeg.y * 3.14159265358979323846f / 180.0f;
  const float rz = rotationDeg.z * 3.14159265358979323846f / 180.0f;

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

struct RayBoxHit final {
  bool hit = false;
  float distance = 0.0f;
  math::Vec3 normal{0.0f, 1.0f, 0.0f};
};

RayBoxHit rayAabb(const math::Vec3& origin, const math::Vec3& direction, const Aabb& box, float maxDistance) {
  const float epsilon = 1e-6f;
  float tMin = 0.0f;
  float tMax = maxDistance;
  math::Vec3 hitNormal{0.0f, 1.0f, 0.0f};

  auto slab = [&](float o, float d, float minB, float maxB, const math::Vec3& nMin, const math::Vec3& nMax) -> bool {
    if (std::fabs(d) < epsilon) {
      return o >= minB && o <= maxB;
    }
    const float inv = 1.0f / d;
    float t1 = (minB - o) * inv;
    float t2 = (maxB - o) * inv;
    math::Vec3 n1 = nMin;
    math::Vec3 n2 = nMax;
    if (t1 > t2) {
      std::swap(t1, t2);
      std::swap(n1, n2);
    }
    if (t1 > tMin) {
      tMin = t1;
      hitNormal = n1;
    }
    tMax = std::min(tMax, t2);
    return tMin <= tMax;
  };

  if (!slab(origin.x, direction.x, box.min.x, box.max.x, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f})) return {};
  if (!slab(origin.y, direction.y, box.min.y, box.max.y, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f})) return {};
  if (!slab(origin.z, direction.z, box.min.z, box.max.z, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f})) return {};

  if (tMax < 0.0f) return {};
  if (tMin < 0.0f) tMin = 0.0f;

  RayBoxHit out;
  out.hit = true;
  out.distance = tMin;
  out.normal = hitNormal;
  return out;
}

}  // namespace

namespace ecs::systems {

void RayDetectionSystem::tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) {
  (void)timeSeconds;
  struct Body final {
    ecs::EntityId id = ecs::kInvalidEntityId;
    Aabb bounds{};
    ecs::ColliderComponent collider{};
    ecs::TransformComponent transform{};
  };

  std::unordered_map<ecs::EntityId, Body> bodies;
  m_grid.clear();

  registry.view<ecs::ColliderComponent, ecs::TransformComponent>([&](ecs::EntityId id,
                                                                      const ecs::ColliderComponent& collider,
                                                                      const ecs::TransformComponent& tr) {
    if (collider.isTrigger || collider.shape == ecs::ColliderComponent::Shape::Terrain) return;

    Body body;
    body.id = id;
    body.collider = collider;
    body.transform = tr;
    body.bounds = colliderAabb(tr, collider);
    bodies.emplace(id, body);
    m_grid.insert({id, body.bounds});
  });

  registry.view<ecs::TerrainComponent, ecs::ColliderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, const ecs::TerrainComponent& terrain, const ecs::ColliderComponent& collider, const ecs::TransformComponent& tr) {
        if (collider.shape != ecs::ColliderComponent::Shape::Terrain) return;

        const float halfX = static_cast<float>(terrain.gridWidth) * terrain.cellSizeMeters * 0.5f;
        const float halfZ = static_cast<float>(terrain.gridHeight) * terrain.cellSizeMeters * 0.5f;
        const float halfY = std::max(terrain.heightScaleMeters, collider.terrain.thicknessMeters);

        Body body;
        body.id = id;
        body.collider = collider;
        body.transform = tr;
        body.bounds = {{tr.position.x - halfX, tr.position.y - halfY, tr.position.z - halfZ},
                       {tr.position.x + halfX, tr.position.y + halfY, tr.position.z + halfZ}};
        bodies.emplace(id, body);
        m_grid.insert({id, body.bounds});
      });

  registry.view<ecs::RaycastComponent, ecs::TransformComponent>([&](ecs::EntityId rayId,
                                                                     ecs::RaycastComponent& ray,
                                                                     const ecs::TransformComponent& tr) {
    if (!ray.enabled || ray.length <= 0.0f) return;

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
        direction = eulerToForward(tr.rotation);
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

    const math::Vec3 end = origin + direction * ray.length;
    const Aabb swept{math::Vec3{std::min(origin.x, end.x), std::min(origin.y, end.y), std::min(origin.z, end.z)},
                     math::Vec3{std::max(origin.x, end.x), std::max(origin.y, end.y), std::max(origin.z, end.z)}};

    std::unordered_set<ecs::EntityId> candidates;
    auto gather = [&](const std::vector<ecs::EntityId>& ids) {
      for (ecs::EntityId id : ids) {
        candidates.insert(id);
      }
    };
    gather(m_grid.entitiesAlongRay(origin, direction, ray.length));
    if (ray.radius > 0.0f) {
      gather(m_grid.queryAabb(expanded(swept, ray.radius)));
    }

    std::vector<physics::RaycastHit> hits;
    hits.reserve(candidates.size());

    const ecs::EntityId sensorParentEntity =
        ray.ignoreSelf && registry.isAlive(ray.sensorEntity)
            ? [&]() {
                const auto* sensor = registry.tryGet<ecs::SensorComponent>(ray.sensorEntity);
                return sensor ? sensor->parentEntity : ecs::kInvalidEntityId;
              }()
            : ecs::kInvalidEntityId;

    for (ecs::EntityId candidateId : candidates) {
      if (ray.ignoreSelf &&
          (candidateId == ray.originEntity || candidateId == ray.sensorEntity || candidateId == sensorParentEntity)) {
        continue;
      }
      if (!registry.isAlive(candidateId)) continue;
      const auto bodyIt = bodies.find(candidateId);
      if (bodyIt == bodies.end()) continue;

      const auto& collider = bodyIt->second.collider;
      if (ray.ignoreTriggerColliders && collider.isTrigger) continue;
      if ((ray.ignoreLayers & collider.collisionLayer) != 0u) continue;
      if ((ray.collisionLayers & collider.collisionLayer) == 0u) continue;

      Aabb target = bodyIt->second.bounds;
      if (ray.radius > 0.0f) {
        target = expanded(target, ray.radius);
      }

      const RayBoxHit boxHit = rayAabb(origin, direction, target, ray.length);
      if (!boxHit.hit) continue;

      physics::RaycastHit hit{};
      hit.hasHit = true;
      hit.hitEntityId = candidateId;
      hit.hitNormal = boxHit.normal;
      hit.hitPosition = origin + direction * boxHit.distance;
      hit.distance = boxHit.distance;
      hit.hitTime = ray.length > 0.0f ? static_cast<double>(boxHit.distance / ray.length) : 0.0;
      hits.push_back(hit);
    }

    std::sort(hits.begin(), hits.end(), [](const physics::RaycastHit& a, const physics::RaycastHit& b) {
      return a.distance < b.distance;
    });

    ray.hitResults = hits;
    const int maxHits = std::max(1, ray.maxHits);
    const int hitCount = std::min(maxHits, static_cast<int>(hits.size()));
    for (int i = 0; i < hitCount; ++i) {
      events.emit<ecs::events::RayHitEvent>({rayId, ray.sensorEntity, hits[static_cast<std::size_t>(i)].hitEntityId, ray.raycastCategory,
                                             hits[static_cast<std::size_t>(i)]});
    }
  });
}

}  // namespace ecs::systems
