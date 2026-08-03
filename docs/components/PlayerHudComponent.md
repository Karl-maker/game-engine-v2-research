# PlayerHudComponent

## What it is

- PlayerHudComponent (descriptive only)
- Marks a screen-space HUD entity for the local player that renders a textured overlay in the bottom-left.

## How it fits

- `PlayableCharacterFactory` creates a `player_hud` entity with `PlayerHudComponent`.
- `PlayerHudSystem` configures a `Draw2DComponent` with layered quads:
  - `player_health_base` (HUD texture)
  - `player_health_fill` (placeholder colored quad you can drive manually)

## Key fields

- `targetEntity` — the player whose stats the HUD should reflect.
- `texturePath` — texture path (default `assets/hud/player-health.png`).
- `sizePx` — on-screen size in pixels.
- `marginLeftPx` / `marginBottomPx` — bottom-left padding.
- `tint` — base tint applied to the HUD quad.
