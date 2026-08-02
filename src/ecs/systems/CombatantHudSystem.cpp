// Author: Karl-Johan Bailey

#include "ecs/systems/CombatantHudSystem.h"

#include "ecs/components/AttachmentComponent.h"
#include "ecs/components/BillboardComponent.h"
#include "ecs/components/CombatantHudComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/services/SvgDrawService.h"

namespace ecs::systems {

void CombatantHudSystem::tick(EntityRegistry& registry) const {
  registry.view<ecs::CombatantHudComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, ecs::CombatantHudComponent& hud, ecs::TransformComponent& tr) {
        if (!hud.enabled) return;
        if (hud.targetEntity == ecs::kInvalidEntityId || !registry.isAlive(hud.targetEntity)) return;

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

        // Ensure billboard uses the SVG texture and has the correct aspect-derived size.
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
        billboard->texture = ecs::services::SvgDrawService::textureRef(hud.svgPath, 256);

        ecs::services::SvgSizeRequest sizeReq;
        sizeReq.targetHeightMeters = hud.heightMeters;
        const auto size = ecs::services::SvgDrawService::computeSize(hud.svgPath, sizeReq);
        if (size.valid) {
          billboard->sizeMeters = size.sizeMeters;
        } else {
          billboard->sizeMeters = {hud.heightMeters, hud.heightMeters};
        }

        // If this HUD entity was created without a transform, ensure it's non-zero scale.
        if (tr.scale.x == 0.0f && tr.scale.y == 0.0f && tr.scale.z == 0.0f) {
          tr.scale = {1.0f, 1.0f, 1.0f};
        }
      });
}

}  // namespace ecs::systems
