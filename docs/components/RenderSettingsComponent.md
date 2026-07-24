# RenderSettingsComponent

## Purpose

- RenderSettingsComponent (demo)
- Global-ish render switches for the demo renderer.

## Use when

- Use this for per-scene or demo-wide rendering switches like shadows, tessellation, and debug overlays.

## Key fields

- `enabled` — Enable or disable this part of the component.
- `shadowsEnabled` — Master switch for shadow rendering.
- `shadowQuality` — Renderer-defined quality tier for shadow maps or shadow detail.
- `shadowStrength` — How dark the shadowing should look.
- `shadowUseTessellation` — Whether tessellation should be used in the shadow caster pass when supported.
- `showRays` — Draw debug ray queries.
- `showCollisionBoxes` — Draw physics collision bounds.
- `showCombatBoxes` — Draw hit/hurt/combat volumes.
- `showSkeletonBones` — Draw bone debug overlays for skinned entities.

## Example

```cpp
auto& renderSettings = registry.emplace<ecs::RenderSettingsComponent>(entity);
renderSettings.shadowsEnabled = true;
renderSettings.showCollisionBoxes = true;
```

## Notes

- Treat this as a scene-level configuration bucket rather than a per-actor gameplay component.
- If you expose more render toggles later, keep them here so the rest of the scene can read one consistent settings source.
