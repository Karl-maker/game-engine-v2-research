# CombatantHudComponent

## What it is

- CombatantHudComponent (descriptive only)
- Marks a world-space HUD entity that follows a combatant and renders a camera-facing textured billboard.

## How it fits

- `CombatantFactory` creates a HUD entity per combatant and attaches it via `AttachmentComponent`.
- `CombatantHudSystem` keeps the entity configured (target, offset, texture, size).
- Rendering uses `BillboardComponent` + the OpenGL renderer's billboard pass.

## Key fields

- `targetEntity` — the combatant whose stats the HUD should reflect.
- `texturePath` — texture path (default `assets/hud/combatant-health.png`).
- `worldOffset` — placement above the combatant (meters).
- `sizeMeters` — world-space size of the billboard (meters).
- `tint` — base tint applied to the texture.
