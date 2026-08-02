# CombatantHudComponent

## What it is

- CombatantHudComponent (descriptive only)
- Marks a world-space HUD entity that follows a combatant and renders a camera-facing SVG billboard.

## How it fits

- `CombatantFactory` creates a HUD entity per combatant and attaches it via `AttachmentComponent`.
- `CombatantHudSystem` keeps the entity configured (target, offset, SVG, size).
- Rendering uses `BillboardComponent` + the OpenGL renderer's billboard pass.

## Key fields

- `targetEntity` — the combatant whose stats the HUD should reflect.
- `svgPath` — SVG texture path (default `assets/hud/combatant-health.svg`).
- `worldOffset` — placement above the combatant (meters).
- `heightMeters` — billboard height; width is derived from SVG aspect ratio via `SvgDrawService`.
- `tint` — base tint applied to the texture.

