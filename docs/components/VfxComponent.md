# VfxComponent

## What it is

- VfxComponent (descriptive only)
- Describes a visual effect emitter such as fire, electricity, sparks, smoke, or steam.
- Rendering systems interpret these settings and apply LOD / quality-based budgets.

## When to use it

- Use this for effects that need quality levels, distance culling, and LOD-specific budgets.

## How it fits

- Treat `VfxComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Choose the effect `type` and `quality` first.
- Set `maxRenderDistance` and LOD thresholds so distant effects can downgrade or disappear.
- Tune spawn rate, lifetime, and motion values to match the effect category.

## Effect recipes

- **Fire:** keep `upwardBias` high, add flicker, and let `heightMeters` define the plume shape.
- **Electricity:** raise `chargeLengthMeters`, use a moderate `branchCount`, and keep `pulseSpeed` high enough to feel alive.
- **Sparks:** raise `sparkCount`, shorten `sparkFadeSeconds`, and keep the trail length small for punchy bursts.
- **Smoke:** lower motion speed, increase lifetime, and use larger `sizeMeters` values for soft expansion.
- **Steam:** use lighter color values, modest upward motion, and a soft falloff so the effect reads as vapor rather than smoke.

## LOD and quality

- `Quality::Low` should keep the effect readable with the fewest particles or cheapest samples.
- `Quality::Medium` is the default balance point for most gameplay scenes.
- `Quality::High` should be the target for close-up effects that matter to the player.
- `Quality::Ultra` is for showcase moments or scenes where the effect is a focal point.
- Use `lodForceNearDistance` to stop the effect from degrading too early when the camera is close.
- Use `maxRenderDistance` to cull the effect entirely when it is too far away to matter.

## Enums

- `Type`: `Fire`, `Electricity`, `Sparks`, `Smoke`, `Steam`
- `Quality`: `Low`, `Medium`, `High`, `Ultra`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `type` — Enum or bitmask value that changes system behavior.
- `quality` — Enum or bitmask value that changes system behavior.
- `autoQuality` — Master on/off switch or similar behavior flag.
- `maxRenderDistance` — Size, reach, or distance tuning.
- `lodNearDistance` — Size, reach, or distance tuning.
- `lodMidDistance` — Stable reference used by content, loaders, or rendering systems.
- `lodFarDistance` — Size, reach, or distance tuning.
- `lodUltraDistance` — Size, reach, or distance tuning.
- `lodForceNearDistance` — Size, reach, or distance tuning.
- `viewDotBias` — Field of type `float` consumed by systems that read this component.
- `intensity` — Numeric tuning used by gameplay or rendering systems.
- `spawnRate` — Field of type `float` consumed by systems that read this component.
- `burstInterval` — Color, tint, or display styling.
- `lifetimeSeconds` — Field of type `float` consumed by systems that read this component.
- `sizeMeters` — Size, reach, or distance tuning.
- `sizeVariance` — Size, reach, or distance tuning.
- `speedMetersPerSecond` — Numeric tuning used by gameplay or rendering systems.
- `speedVariance` — Numeric tuning used by gameplay or rendering systems.
- `gravityScale` — Size, reach, or distance tuning.
- `drag` — Numeric tuning used by gameplay or rendering systems.
- `flickerStrength` — Numeric tuning used by gameplay or rendering systems.
- `flickerSpeed` — Numeric tuning used by gameplay or rendering systems.
- `looping` — Master on/off switch or similar behavior flag.
- `castLight` — Field of type `bool` consumed by systems that read this component.
- `seed` — Field of type `std::uint32_t` consumed by systems that read this component.
- `heightMeters` — Size, reach, or distance tuning.
- `upwardBias` — Field of type `float` consumed by systems that read this component.
- `spreadRadiusMeters` — Size, reach, or distance tuning.
- `heatHazeStrength` — Numeric tuning used by gameplay or rendering systems.
- `chargeLengthMeters` — Size, reach, or distance tuning.
- `arcJitter` — Field of type `float` consumed by systems that read this component.
- `branchCount` — Count or budget control for work done by the system.
- `segmentCount` — Count or budget control for work done by the system.
- `pulseSpeed` — Numeric tuning used by gameplay or rendering systems.
- `sparkCount` — Count or budget control for work done by the system.
- `sparkSpreadDegrees` — Field of type `float` consumed by systems that read this component.
- `sparkTrailLengthMeters` — Size, reach, or distance tuning.
- `sparkFadeSeconds` — Field of type `float` consumed by systems that read this component.
- `primaryColor` — Color, tint, or display styling.
- `secondaryColor` — Color, tint, or display styling.

## Example

```cpp
auto& component = registry.emplace<ecs::VfxComponent>(entity);
component = ecs::VfxComponent{};
```

## Common pairings

- Use `autoQuality` when the renderer should decide the best cost level automatically.

## Notes

- Keep `VfxComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
