// Author: Karl-Johan Bailey

#include "ecs/systems/PlayerHudSystem.h"

#include "ecs/components/Draw2DComponent.h"
#include "ecs/components/PlayerHudComponent.h"
#include "ecs/services/SvgDrawService.h"

namespace ecs::systems {

namespace {

static ecs::Draw2DComponent::Quad* findQuadByName(ecs::Draw2DComponent& draw, const std::string& name) {
  for (auto& q : draw.quads) {
    if (q.name == name) return &q;
  }
  return nullptr;
}

}  // namespace

void PlayerHudSystem::tick(EntityRegistry& registry) const {
  registry.view<ecs::PlayerHudComponent>([&](ecs::EntityId id, ecs::PlayerHudComponent& hud) {
    if (!hud.enabled) return;

    auto* draw2d = registry.tryGet<ecs::Draw2DComponent>(id);
    if (!draw2d) draw2d = &registry.emplace<ecs::Draw2DComponent>(id);
    draw2d->enabled = true;

    ecs::Draw2DComponent::Quad* base = findQuadByName(*draw2d, "player_health_base");
    if (!base) {
      draw2d->quads.push_back({});
      base = &draw2d->quads.back();
      base->name = "player_health_base";
    }

    ecs::services::SvgSizeRequest sizeReq;
    sizeReq.targetHeightPx = hud.heightPx;
    const auto size = ecs::services::SvgDrawService::computeSize(hud.svgPath, sizeReq);

    base->enabled = true;
    base->layer = 0;
    base->anchor = ecs::Draw2DComponent::Anchor::BottomLeft;
    base->offsetPx = {hud.marginLeftPx, hud.marginBottomPx};
    base->sizePx = size.valid ? size.sizePx : math::Vec2{hud.heightPx, hud.heightPx};
    base->textureEnabled = true;
    base->texture = ecs::services::SvgDrawService::textureRef(hud.svgPath, 512);
    base->uv0 = {0.0f, 0.0f};
    base->uv1 = {1.0f, 1.0f};
    base->colorTL = hud.tint;
    base->colorTR = hud.tint;
    base->colorBR = hud.tint;
    base->colorBL = hud.tint;

    ecs::Draw2DComponent::Quad* fill = findQuadByName(*draw2d, "player_health_fill");
    if (!fill) {
      draw2d->quads.push_back({});
      fill = &draw2d->quads.back();
      fill->name = "player_health_fill";
    }

    // Placeholder dynamic element the user can position/drive manually.
    fill->enabled = true;
    fill->layer = 1;
    fill->anchor = ecs::Draw2DComponent::Anchor::BottomLeft;
    fill->offsetPx = {hud.marginLeftPx + 24.0f, hud.marginBottomPx + 28.0f};
    fill->sizePx = {140.0f, 14.0f};
    fill->textureEnabled = false;
    const render::Color green{0.10f, 0.95f, 0.25f, 0.90f};
    fill->colorTL = green;
    fill->colorTR = green;
    fill->colorBR = green;
    fill->colorBL = green;
  });
}

}  // namespace ecs::systems
