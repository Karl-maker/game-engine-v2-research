# LightComponent

## What it is

- LightComponent (descriptive only)
- Stores lighting parameters. Rendering systems interpret these values.

## When to use it

- Use this for directional, point, spot, or area-style lights that the renderer consumes.

## How it fits

- Treat `LightComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Pick the light type first.
- Set color and intensity before adjusting shadows or volumetrics.
- Tune range, radius, and cone angles to match the scene scale.

## Enums

- `Type`: `Directional`, `Point`, `Spot`, `Area`, `// optional/engine-dependent`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `type` — Enum or bitmask value that changes system behavior.
- `color` — Color, tint, or display styling.
- `intensity` — Numeric tuning used by gameplay or rendering systems.
- `hasTemperature` — Field of type `bool` consumed by systems that read this component.
- `temperatureKelvin` — Field of type `float` consumed by systems that read this component.
- `range` — Size, reach, or distance tuning.
- `radius` — Size, reach, or distance tuning.
- `direction` — Spatial placement or directional tuning.
- `spotInnerAngleDeg` — Orientation or angular tuning.
- `spotOuterAngleDeg` — Orientation or angular tuning.
- `castShadows` — Master on/off switch or similar behavior flag.
- `shadowResolution` — Field of type `std::uint32_t` consumed by systems that read this component.
- `shadowBias` — Field of type `float` consumed by systems that read this component.
- `normalBias` — Field of type `float` consumed by systems that read this component.
- `shadowDistance` — Size, reach, or distance tuning.
- `volumetricLighting` — Field of type `bool` consumed by systems that read this component.
- `volumetricIntensity` — Numeric tuning used by gameplay or rendering systems.
- `affectsLayers` — Enum or bitmask value that changes system behavior.
- `lightMask` — Enum or bitmask value that changes system behavior.
- `indirectMultiplier` — Field of type `float` consumed by systems that read this component.
- `specularIntensity` — Numeric tuning used by gameplay or rendering systems.
- `diffuseIntensity` — Numeric tuning used by gameplay or rendering systems.
- `hasCookieTexture` — Stable reference used by content, loaders, or rendering systems.
- `cookieTextureId` — Stable reference used by content, loaders, or rendering systems.
- `hasLensFlare` — Field of type `bool` consumed by systems that read this component.
- `lensFlareId` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::LightComponent>(entity);
component = ecs::LightComponent{};
```

## Common pairings

- If shadows look noisy, reduce bias problems before increasing resolution.

## Notes

- Keep `LightComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
