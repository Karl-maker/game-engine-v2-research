# HudComponent

## Purpose

- HudComponent (descriptive only)
- Describes screen-space or world-space HUD widgets.
- A widget can be a panel, image, text label, or progress bar.

## Use when

- Use this for on-screen HUD and world-space UI, including portraits, bars, labels, and floating health values.

## Enums

- `Space`: `Screen`, `World`
- `Kind`: `Panel`, `Image`, `Text`, `Bar`
- `ValueSource`: `Manual`, `StatsHealth`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `name` — A stable content or debug identifier.
- `space` — Field of type `Space` used by systems that consume this component.
- `kind` — Field of type `Kind` used by systems that consume this component.
- `valueSource` — Reference to another entity or input source.
- `positionPx` — Spatial placement or relative offset.
- `sizePx` — Size, reach, or distance tuning.
- `worldOffset` — Spatial placement or relative offset.
- `sizeMeters` — Size, reach, or distance tuning.
- `billboard` — Field of type `bool` used by systems that consume this component.
- `sourceEntity` — Reference to another entity or input source.
- `value` — Numeric value used by a system or widget.
- `maxValue` — Numeric value used by a system or widget.
- `showValueText` — Display text used by UI or debug output.
- `label` — Display text used by UI or debug output.
- `text` — Display text used by UI or debug output.
- `textScalePx` — Size, reach, or distance tuning.
- `showBackground` — Field of type `bool` used by systems that consume this component.
- `showBorder` — Field of type `bool` used by systems that consume this component.
- `borderThicknessPx` — Field of type `float` used by systems that consume this component.
- `texture` — Texture or material binding.
- `textureEnabled` — Enable or disable this part of the component.
- `tint` — Color or tint control.
- `backgroundColor` — Color or tint control.
- `borderColor` — Color or tint control.
- `fillColor` — Color or tint control.
- `fillBackgroundColor` — Color or tint control.
- `textColor` — Color or tint control.
- `widgets` — Field of type `std::vector<Widget>` used by systems that consume this component.

## Example

```cpp
auto& hud = registry.emplace<ecs::HudComponent>(entity);

ecs::HudComponent::Widget lifeBar;
lifeBar.kind = ecs::HudComponent::Kind::Bar;
lifeBar.space = ecs::HudComponent::Space::Screen;
lifeBar.positionPx = {96.0f, 24.0f};
lifeBar.sizePx = {320.0f, 24.0f};
lifeBar.sourceEntity = player;
lifeBar.valueSource = ecs::HudComponent::ValueSource::StatsHealth;
lifeBar.label = "Life";
lifeBar.showValueText = true;
hud.widgets.push_back(lifeBar);
```

## Notes

- Use `textureEnabled` and `texture` for portraits, icons, panel frames, or any image-backed widget.
- Use `text` for custom strings and update it each frame if the displayed value changes often.
- Use `space = World` plus `billboard = true` for floating labels above actors or objects.
