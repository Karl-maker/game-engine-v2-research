# HudComponent

## What it is

- HudComponent (descriptive only)
- Describes screen-space or world-space HUD widgets.
- A widget can be a panel, image, text label, or progress bar.

## When to use it

- Use this for on-screen UI or floating 3D labels that the graphics engine should draw.

## How it fits

- Treat `HudComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Choose `space = Screen` for classic HUD and `space = World` for in-world labels or markers.
- Choose `kind = Image` for portraits/icons, `Panel` for frames, `Bar` for meters, and `Text` for labels.
- Bind `sourceEntity` and `valueSource` when the widget should follow gameplay state such as health.

## Widget recipes

- **Life bar:** use `kind = Bar`, bind it to a character entity, and let `HudSystem` read `StatsComponent::health`.
- **Portrait frame:** use `kind = Image`, turn on `textureEnabled`, and point `texture.key` at the portrait art.
- **Floating nameplate:** use `space = World`, `kind = Text`, and `billboard = true` so it faces the camera.
- **Status panel:** use `kind = Panel` with `showBorder` enabled for a framed container that can hold text or icons.

## Text and value settings

- `text` is the direct string the renderer should draw.
- `label` is the short caption attached to the widget.
- `showValueText` is useful when you want the widget to show the raw number in addition to the bar or label.
- `textScalePx` controls the size of the text in screen-space pixels.

## Texture settings

- Set `textureEnabled = true` only when the widget should actually use an image.
- Use `texture` for icons, portraits, borders, or themed frames.
- Use `tint` to recolor a shared texture without making a new asset.
- Use `backgroundColor`, `borderColor`, `fillColor`, and `fillBackgroundColor` to keep the widget readable even without a texture.

## Enums

- `Space`: `Screen`, `World`
- `Kind`: `Panel`, `Image`, `Text`, `Bar`
- `ValueSource`: `Manual`, `StatsHealth`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `name` — Stable reference used by content, loaders, or rendering systems.
- `space` — Field of type `Space` consumed by systems that read this component.
- `kind` — Field of type `Kind` consumed by systems that read this component.
- `valueSource` — Numeric tuning used by gameplay or rendering systems.
- `positionPx` — Spatial placement or directional tuning.
- `sizePx` — Size, reach, or distance tuning.
- `worldOffset` — Spatial placement or directional tuning.
- `sizeMeters` — Size, reach, or distance tuning.
- `billboard` — Master on/off switch or similar behavior flag.
- `sourceEntity` — Reference to another entity or a linked runtime object.
- `value` — Numeric tuning used by gameplay or rendering systems.
- `maxValue` — Numeric tuning used by gameplay or rendering systems.
- `showValueText` — Color, tint, or display styling.
- `label` — Field of type `std::string` consumed by systems that read this component.
- `text` — Color, tint, or display styling.
- `textScalePx` — Size, reach, or distance tuning.
- `showBackground` — Master on/off switch or similar behavior flag.
- `showBorder` — Master on/off switch or similar behavior flag.
- `borderThicknessPx` — Size, reach, or distance tuning.
- `texture` — Stable reference used by content, loaders, or rendering systems.
- `textureEnabled` — Master on/off switch or similar behavior flag.
- `tint` — Color, tint, or display styling.
- `backgroundColor` — Color, tint, or display styling.
- `borderColor` — Color, tint, or display styling.
- `fillColor` — Color, tint, or display styling.
- `fillBackgroundColor` — Color, tint, or display styling.
- `textColor` — Color, tint, or display styling.
- `widgets` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& hud = registry.emplace<ecs::HudComponent>(entity);

auto lifeBar = ecs::HudComponent::Widget{};
lifeBar.name = "life_bar";
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

## Common pairings

- Use `textureEnabled` plus `texture` for portraits, icon frames, and skinned UI panels.
- Use `text` and `showValueText` for runtime numbers or labels that change often.
- Set `billboard = true` for world-space widgets that should face the camera.

## Notes

- Keep `HudComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
