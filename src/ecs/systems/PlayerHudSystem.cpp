// Author: Karl-Johan Bailey

#include "ecs/systems/PlayerHudSystem.h"

#include "ecs/components/Draw2DComponent.h"
#include "ecs/components/PlayerHudComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/services/ImageDrawService.h"

#include <algorithm>

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

    const ecs::StatsComponent* stats = (hud.targetEntity != ecs::kInvalidEntityId) ? registry.tryGet<ecs::StatsComponent>(hud.targetEntity) : nullptr;
    const float maxHealth = (stats && stats->maxHealth > 0.0f) ? stats->maxHealth : (stats ? stats->health : 1.0f);
    const float ratio = (stats && maxHealth > 0.0f) ? std::clamp(stats->health / maxHealth, 0.0f, 1.0f) : 1.0f;

    ecs::services::ImageSizeRequest sizeReq;
    sizeReq.targetHeightPx = hud.heightPx;
    const auto size = ecs::services::ImageDrawService::computeSize(hud.texturePath, sizeReq);

    base->enabled = true;
    base->layer = 0;
    base->anchor = ecs::Draw2DComponent::Anchor::BottomLeft;
    base->offsetPx = {hud.marginLeftPx, hud.marginBottomPx};
    base->sizePx = size.valid ? size.sizePx : math::Vec2{hud.heightPx, hud.heightPx};
    base->textureEnabled = true;
    base->texture = ecs::services::ImageDrawService::textureRef(hud.texturePath);
    base->uv0 = {hud.flipU ? 1.0f : 0.0f, hud.flipV ? 1.0f : 0.0f};
    base->uv1 = {hud.flipU ? 0.0f : 1.0f, hud.flipV ? 0.0f : 1.0f};
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

    fill->enabled = hud.fillEnabled;
    fill->layer = hud.fillLayer;
    fill->anchor = ecs::Draw2DComponent::Anchor::BottomLeft;

    const float fillMaxWidth = std::max(0.0f, base->sizePx.x * std::clamp(hud.fillWidthRatio, 0.0f, 1.0f));
    const float fillHeight = std::max(0.0f, base->sizePx.y * std::clamp(hud.fillHeightRatio, 0.0f, 1.0f));
    const float fillWidth = fillMaxWidth * ratio;

    const float leftInset = (base->sizePx.x - fillMaxWidth) * 0.5f;
    const float bottomInset = (base->sizePx.y - fillHeight) * 0.5f;
    const float fillX = hud.fillFromRight ? (leftInset + (fillMaxWidth - fillWidth)) : leftInset;

    fill->offsetPx = {hud.marginLeftPx + hud.fillOffsetPx.x + fillX, hud.marginBottomPx + hud.fillOffsetPx.y + bottomInset};
    fill->sizePx = {fillWidth, fillHeight};
    fill->textureEnabled = false;
    fill->colorTL = hud.fillColor;
    fill->colorTR = hud.fillColor;
    fill->colorBR = hud.fillColor;
    fill->colorBL = hud.fillColor;
  });
}

}  // namespace ecs::systems
