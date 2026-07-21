#include "ecs/services/RaycastConeFactoryService.h"

// Author: Karl-Johan Bailey

#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/RaycastComponent.h"
#include "ecs/components/TransformComponent.h"

#include <algorithm>
#include <cmath>

namespace ecs::services {

static float degToRad(float d) { return d * 3.14159265358979323846f / 180.0f; }

static math::Vec3 normalizeSafe(const math::Vec3& v) {
  const float len = math::length(v);
  if (len <= 0.000001f) return {0.0f, 0.0f, 1.0f};
  return v * (1.0f / len);
}

std::vector<EntityId> RaycastConeFactoryService::createCone(EntityRegistry& registry,
                                                            EntityId owner,
                                                            const RaycastConeConfig& cfg) {
  std::vector<EntityId> out;
  if (!registry.isAlive(owner)) return out;
  if (cfg.rayCount <= 0) return out;

  out.reserve(static_cast<std::size_t>(cfg.rayCount));

  // Build rays distributed in a cone around +Z (local forward).
  const float maxAngleRad = degToRad(std::max(0.0f, cfg.coneAngleDeg));
  const float cosMax = std::cos(maxAngleRad);
  const float golden = 2.39996322972865332f;  // ~ pi * (3 - sqrt(5))

  for (int i = 0; i < cfg.rayCount; ++i) {
    const float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(cfg.rayCount);  // 0..1
    const float theta = golden * static_cast<float>(i);
    const float cosAlpha = (1.0f - t) * 1.0f + t * cosMax;
    const float sinAlpha = std::sqrt(std::max(0.0f, 1.0f - cosAlpha * cosAlpha));

    // Cone direction around +Z.
    math::Vec3 dirLocal{std::cos(theta) * sinAlpha, std::sin(theta) * sinAlpha, cosAlpha};
    dirLocal = normalizeSafe(dirLocal);

    const std::string name = cfg.baseName + "_" + std::to_string(i);
    const EntityId rayEntity = registry.createEntity(name);
    registry.emplace<TransformComponent>(rayEntity);

    // Attach the ray entity to the owner so it inherits rotation/position.
    auto& attach = registry.emplace<AttachmentComponent>(rayEntity);
    AttachmentComponent::Attachment a;
    a.targetEntity = owner;
    a.mode = AttachmentComponent::Mode::Parent;
    a.enabled = true;
    a.positionOffset = cfg.originLocalOffset;
    a.inheritPosition = true;
    a.inheritRotation = true;
    a.inheritScale = false;
    a.space = AttachmentComponent::Space::Local;
    attach.attachments.push_back(a);

    // Describe the raycast. A raycast system can transform this local direction
    // by the ray entity's (inherited) rotation later.
    auto& rc = registry.emplace<RaycastComponent>(rayEntity);
    rc.enabled = true;
    rc.originMode = RaycastComponent::OriginMode::Entity;
    rc.originEntity = rayEntity;  // origin comes from this ray entity's transform
    rc.localOffset = {0.0f, 0.0f, 0.0f};
    rc.directionMode = RaycastComponent::DirectionMode::CustomVector;
    rc.customDirection = dirLocal;
    rc.length = cfg.length;
    rc.radius = cfg.radius;
    rc.collisionLayers = cfg.collisionLayers;
    rc.ignoreLayers = cfg.ignoreLayers;
    rc.ignoreTriggerColliders = cfg.ignoreTriggerColliders;
    rc.ignoreSelf = cfg.ignoreSelf;
    rc.maxHits = cfg.maxHits;

    out.push_back(rayEntity);
  }

  return out;
}

}  // namespace ecs::services

