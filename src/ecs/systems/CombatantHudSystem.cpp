// Author: Karl-Johan Bailey

#include "ecs/systems/CombatantHudSystem.h"

#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/BillboardComponent.h"
#include "ecs/components/CombatantHudComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/CameraComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/services/ImageDrawService.h"

#include <algorithm>
#include <cmath>

namespace ecs::systems {

namespace {

static math::Vec3 cross(const math::Vec3& a, const math::Vec3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

static math::Vec3 forwardFromPitchYawDeg(const math::Vec3& rotationDeg) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kDegToRad = kPi / 180.0f;
  const float pitch = rotationDeg.x * kDegToRad;
  const float yaw = rotationDeg.y * kDegToRad;
  const math::Vec3 fwd{
      std::cos(pitch) * std::sin(yaw),
      -std::sin(pitch),
      std::cos(pitch) * std::cos(yaw),
  };
  return math::normalize(fwd);
}

}  // namespace

void CombatantHudSystem::tick(EntityRegistry& registry) const {
  // Pick the first camera as the reference for subtle distance scaling / depth bias.
  const ecs::TransformComponent* camTr = nullptr;
  registry.view<ecs::CameraComponent, ecs::TransformComponent>([&](ecs::EntityId, const ecs::CameraComponent&, const ecs::TransformComponent& tr) {
    if (!camTr) camTr = &tr;
  });
  const math::Vec3 camPos = camTr ? camTr->position : math::Vec3{};
  const math::Vec3 camFwd = camTr ? forwardFromPitchYawDeg(camTr->rotation) : math::Vec3{0.0f, 0.0f, 1.0f};
  math::Vec3 camRight = math::normalize(cross(camFwd, {0.0f, 1.0f, 0.0f}));
  if (math::lengthSq(camRight) < 1e-6f) camRight = {1.0f, 0.0f, 0.0f};
  const math::Vec3 camUp = math::normalize(cross(camRight, camFwd));

  registry.view<ecs::CombatantHudComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, ecs::CombatantHudComponent& hud, ecs::TransformComponent& tr) {
        if (!hud.enabled) return;
        if (hud.targetEntity == ecs::kInvalidEntityId || !registry.isAlive(hud.targetEntity)) return;

        const ecs::TransformComponent* targetTr = registry.tryGet<ecs::TransformComponent>(hud.targetEntity);
        const float dist = (camTr && targetTr) ? math::length(camPos - targetTr->position) : 0.0f;
        float scale = 1.0f;
        if (hud.distanceScaleEnabled && camTr) {
          const float start = hud.distanceScaleStartMeters;
          const float end = std::max(start + 0.001f, hud.distanceScaleEndMeters);
          const float t = std::clamp((dist - start) / (end - start), 0.0f, 1.0f);
          scale = 1.0f + t * (std::max(1.0f, hud.distanceScaleAtEnd) - 1.0f);
        }

        // Ensure attachment follows the combatant so the HUD keeps its own transform.
        auto* attachment = registry.tryGet<ecs::AttachmentComponent>(id);
        if (!attachment) attachment = &registry.emplace<ecs::AttachmentComponent>(id);
        if (attachment->attachments.empty()) attachment->attachments.push_back({});
        auto& a = attachment->attachments[0];
        a.enabled = true;
        a.mode = ecs::AttachmentComponent::Mode::Parent;
        a.targetEntity = hud.targetEntity;
        a.positionOffset = hud.worldOffset;
        a.inheritPosition = true;
        a.inheritRotation = false;
        a.inheritScale = false;

        // Base billboard (frame).
        auto* billboard = registry.tryGet<ecs::BillboardComponent>(id);
        if (!billboard) billboard = &registry.emplace<ecs::BillboardComponent>(id);
        billboard->enabled = true;
        billboard->visible = true;
        billboard->textureEnabled = true;
        billboard->faceMode = ecs::BillboardComponent::FaceMode::CameraPlane;
        billboard->pivot = {0.5f, 0.0f};
        billboard->worldOffset = {0.0f, 0.0f, 0.0f};
        billboard->rotationOffsetDeg = {0.0f, 0.0f, 0.0f};
        billboard->tint = hud.tint;
        billboard->texture = ecs::services::ImageDrawService::textureRef(hud.texturePath);
        billboard->maxRenderDistance = hud.maxRenderDistanceMeters;

        ecs::services::ImageSizeRequest sizeReq;
        sizeReq.targetHeightMeters = hud.heightMeters;
        const auto size = ecs::services::ImageDrawService::computeSize(hud.texturePath, sizeReq);
        const math::Vec2 baseRaw = size.valid ? size.sizeMeters : math::Vec2{hud.heightMeters, hud.heightMeters};
        const math::Vec2 baseSize{baseRaw.x * scale, baseRaw.y * scale};
        billboard->sizeMeters = baseSize;

        // Optional fill billboard in front.
        if (hud.fillEnabled) {
          if (hud.fillEntity == ecs::kInvalidEntityId || !registry.isAlive(hud.fillEntity)) {
            const std::string baseName = registry.identity(id).name;
            hud.fillEntity = registry.createEntity(baseName.empty() ? "combatant_hud_fill" : (baseName + "_fill"));
            registry.emplace<ecs::TransformComponent>(hud.fillEntity);
          }

          auto* fillAttachment = registry.tryGet<ecs::AttachmentComponent>(hud.fillEntity);
          if (!fillAttachment) fillAttachment = &registry.emplace<ecs::AttachmentComponent>(hud.fillEntity);
          if (fillAttachment->attachments.empty()) fillAttachment->attachments.push_back({});
          auto& fa = fillAttachment->attachments[0];
          fa.enabled = true;
          fa.mode = ecs::AttachmentComponent::Mode::Parent;
          fa.targetEntity = hud.targetEntity;
          fa.inheritPosition = true;
          fa.inheritRotation = false;
          fa.inheritScale = false;

          const ecs::StatsComponent* stats = registry.tryGet<ecs::StatsComponent>(hud.targetEntity);
          const float maxHealth = (stats && stats->maxHealth > 0.0f) ? stats->maxHealth : (stats ? stats->health : 1.0f);
          const float ratio = (stats && maxHealth > 0.0f) ? std::clamp(stats->health / maxHealth, 0.0f, 1.0f) : 1.0f;

          const float fillMaxWidth = std::max(0.0f, baseSize.x * std::clamp(hud.fillWidthRatio, 0.0f, 1.0f));
          const float fillHeight = std::max(0.0f, baseSize.y * std::clamp(hud.fillHeightRatio, 0.0f, 1.0f));
          const float fillWidth = fillMaxWidth * ratio;
          const float leftInset = (baseSize.x - fillMaxWidth) * 0.5f;
          const float bottomInset = (baseSize.y - fillHeight) * 0.5f;

          // Base pivot is bottom-center, so bottom-left sits at (-baseSize.x/2, 0) in billboard space.
          const math::Vec3 localOffset =
              camRight * (-(baseSize.x * 0.5f) + leftInset) + camUp * bottomInset + (camFwd * (-hud.fillDepthBiasMeters));
          fa.positionOffset = hud.worldOffset + localOffset;

          auto* fillBillboard = registry.tryGet<ecs::BillboardComponent>(hud.fillEntity);
          if (!fillBillboard) fillBillboard = &registry.emplace<ecs::BillboardComponent>(hud.fillEntity);
          fillBillboard->enabled = true;
          fillBillboard->visible = true;
          fillBillboard->textureEnabled = false;
          fillBillboard->faceMode = ecs::BillboardComponent::FaceMode::CameraPlane;
          fillBillboard->pivot = {0.0f, 0.0f};
          fillBillboard->worldOffset = {0.0f, 0.0f, 0.0f};
          fillBillboard->rotationOffsetDeg = {0.0f, 0.0f, 0.0f};
          fillBillboard->tint = hud.fillColor;
          fillBillboard->sizeMeters = {fillWidth, fillHeight};
          fillBillboard->maxRenderDistance = hud.maxRenderDistanceMeters;
          fillBillboard->doubleSided = true;
          fillBillboard->depthWrite = false;
        } else if (hud.fillEntity != ecs::kInvalidEntityId && registry.isAlive(hud.fillEntity)) {
          if (auto* fillBillboard = registry.tryGet<ecs::BillboardComponent>(hud.fillEntity)) fillBillboard->visible = false;
        }

        // If this HUD entity was created without a transform, ensure it's non-zero scale.
        if (tr.scale.x == 0.0f && tr.scale.y == 0.0f && tr.scale.z == 0.0f) {
          tr.scale = {1.0f, 1.0f, 1.0f};
        }
      });
}

}  // namespace ecs::systems
