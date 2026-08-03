# Draw2DComponent

## What it is

- Draw2DComponent (descriptive only)
- A list of simple screen-space quads rendered on top of the scene.
- Supports per-corner colors (simple gradients) and optional textures (including SVG assets via ImageIO on macOS).

## When to use it

- Use this for HUD elements, icons, simple panels, or debug overlays that should be anchored to the screen.

## How it fits

- Systems create/mutate the quads (for example `PlayerHudSystem`).
- `GraphicsSystem` snapshots them into `FrameSnapshot::draw2d`.
- `OpenGlRenderer` draws them in layer order (lower first, higher on top).

## Anchoring and layers

- `anchor` chooses which screen corner/center the quad is positioned from.
- `offsetPx` is measured from that anchor.
- `layer` controls draw order; higher layers draw last.

## Gradients and textures

- Set all `color*` corners the same for a solid color/tint.
- Set different corner colors for a simple gradient.
- Set `textureEnabled = true` and assign `texture = {true, "path/to/asset.svg", 0}` to draw a texture.

## Example

```cpp
auto& draw = registry.emplace<ecs::Draw2DComponent>(hudEntity);

ecs::Draw2DComponent::Quad base;
base.name = "player_health_base";
base.layer = 0;
base.anchor = ecs::Draw2DComponent::Anchor::BottomLeft;
base.offsetPx = {24.0f, 24.0f};
base.sizePx = {256.0f, 96.0f};
base.textureEnabled = true;
base.texture = {true, "assets/hud/player-health.png", 0};
draw.quads.push_back(base);
```
