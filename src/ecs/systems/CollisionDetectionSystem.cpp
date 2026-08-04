#include "ecs/systems/CollisionDetectionSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ColliderComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/CollisionEvents.h"
#include "terrain/PerlinNoise2D.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace {

using Aabb = ecs::services::SpatialHashGridService::Aabb;

bool layersCollide(physics::LayerMask a, physics::LayerMask b) {
  if (a == physics::kAllLayers || b == physics::kAllLayers) return true;
  if ((a & b) != 0u) return true;
  // Default matrix: world <-> character should collide.
  const bool aWorld = (a & physics::kLayerWorld) != 0u;
  const bool aChar = (a & physics::kLayerCharacter) != 0u;
  const bool bWorld = (b & physics::kLayerWorld) != 0u;
  const bool bChar = (b & physics::kLayerCharacter) != 0u;
  return (aWorld && bChar) || (aChar && bWorld);
}

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
    case ecs::ColliderComponent::Shape::Box:
    case ecs::ColliderComponent::Shape::Mesh:
    case ecs::ColliderComponent::Shape::Terrain:
    default:
      half = {std::max(0.01f, c.size.x * 0.5f), std::max(0.01f, c.size.y * 0.5f), std::max(0.01f, c.size.z * 0.5f)};
      break;
  }
  return {center - half, center + half};
}

bool intersects(const Aabb& a, const Aabb& b) {
  return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y && a.min.z <= b.max.z &&
         a.max.z >= b.min.z;
}

ecs::events::CollisionDetectionEvent makeAabbContact(ecs::EntityId aId,
                                                     ecs::EntityId bId,
                                                     const Aabb& a,
                                                     const Aabb& b,
                                                     double timeSeconds) {
  const math::Vec3 ac = (a.min + a.max) * 0.5f;
  const math::Vec3 bc = (b.min + b.max) * 0.5f;
  const float ox = std::min(a.max.x - b.min.x, b.max.x - a.min.x);
  const float oy = std::min(a.max.y - b.min.y, b.max.y - a.min.y);
  const float oz = std::min(a.max.z - b.min.z, b.max.z - a.min.z);

  math::Vec3 n{(ac.x >= bc.x) ? 1.0f : -1.0f, 0.0f, 0.0f};
  float depth = ox;
  if (oy < depth) {
    depth = oy;
    n = {0.0f, (ac.y >= bc.y) ? 1.0f : -1.0f, 0.0f};
  }
  if (oz < depth) {
    depth = oz;
    n = {0.0f, 0.0f, (ac.z >= bc.z) ? 1.0f : -1.0f};
  }

  ecs::events::CollisionDetectionEvent e;
  e.entityA = aId;
  e.entityB = bId;
  e.contactPoint = (ac + bc) * 0.5f;
  e.contactNormal = n;
  e.penetrationDepth = std::max(0.0f, depth);
  e.time = timeSeconds;
  return e;
}

float sampleTerrainHeight(ecs::EntityId terrainId,
                          const ecs::TerrainComponent& terrain,
                          const ecs::TransformComponent& terrainTr,
                          const math::Vec3& world) {
  (void)terrainId;
  terrain::PerlinNoise2D noise(terrain.noiseSeed);
  return terrainTr.position.y + noise.sampleFractal(world.x, world.z, terrain.noise) * terrain.heightScaleMeters;
}

}  // namespace

namespace ecs::systems {

void CollisionDetectionSystem::tick(EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) {
  struct Body final {
    ecs::EntityId id = ecs::kInvalidEntityId;
    Aabb aabb{};
    ecs::ColliderComponent collider{};
    ecs::TransformComponent transform{};
  };

  std::unordered_map<ecs::EntityId, Body> bodies;
  m_grid.clear();

  // Include static colliders too (they may not have MotionComponent).
  registry.view<ecs::ColliderComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id,
          const ecs::ColliderComponent& collider,
          const ecs::TransformComponent& tr) {
        if (collider.isTrigger) return;
        if (collider.shape == ecs::ColliderComponent::Shape::Terrain) return;
        Body body;
        body.id = id;
        body.collider = collider;
        body.transform = tr;
        body.aabb = colliderAabb(tr, collider);
        bodies.emplace(id, body);
        m_grid.insert({id, body.aabb});
      });

  for (const auto& [aId, bId] : m_grid.candidatePairs()) {
    const auto aIt = bodies.find(aId);
    const auto bIt = bodies.find(bId);
    if (aIt == bodies.end() || bIt == bodies.end()) continue;
    const auto& a = aIt->second;
    const auto& b = bIt->second;
    // Skip purely static pairs (no MotionComponent on either side).
    if (!registry.tryGet<ecs::MotionComponent>(a.id) && !registry.tryGet<ecs::MotionComponent>(b.id)) continue;
    if (!layersCollide(a.collider.collisionLayer, b.collider.collisionLayer)) continue;
    if (!intersects(a.aabb, b.aabb)) continue;
    events.emit<ecs::events::CollisionDetectionEvent>(makeAabbContact(a.id, b.id, a.aabb, b.aabb, timeSeconds));
  }

  registry.view<ecs::TerrainComponent, ecs::TransformComponent>(
      [&](ecs::EntityId terrainId, const ecs::TerrainComponent& terrain, const ecs::TransformComponent& terrainTr) {
        const float halfW = static_cast<float>(terrain.gridWidth) * terrain.cellSizeMeters * 0.5f;
        const float halfD = static_cast<float>(terrain.gridHeight) * terrain.cellSizeMeters * 0.5f;
        const Aabb terrainBounds{{terrainTr.position.x - halfW, terrainTr.position.y - terrain.heightScaleMeters, terrainTr.position.z - halfD},
                                 {terrainTr.position.x + halfW, terrainTr.position.y + terrain.heightScaleMeters, terrainTr.position.z + halfD}};
        const auto terrainCandidates = m_grid.queryAabb(terrainBounds);

        for (const ecs::EntityId id : terrainCandidates) {
          const auto bodyIt = bodies.find(id);
          if (bodyIt == bodies.end()) continue;
          const auto& body = bodyIt->second;
          const auto* motion = registry.tryGet<ecs::MotionComponent>(id);
          if (!motion) continue;
          const float ground = sampleTerrainHeight(terrainId, terrain, terrainTr, body.transform.position);
          const float bottom = body.aabb.min.y;
          if (bottom >= ground) continue;

          ecs::events::CollisionDetectionEvent e;
          e.entityA = id;
          e.entityB = terrainId;
          e.contactPoint = {body.transform.position.x, ground, body.transform.position.z};
          e.contactNormal = {0.0f, 1.0f, 0.0f};
          e.penetrationDepth = ground - bottom;
          e.time = timeSeconds;
          events.emit<ecs::events::CollisionDetectionEvent>(e);
        }
      });
}

}  // namespace ecs::systems
