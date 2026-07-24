# LightComponent

## Purpose

- LightComponent (descriptive only)
- Stores lighting parameters. Rendering systems interpret these values.

## Use when

- Use this for directional, point, spot, or area-style lighting that the renderer consumes.

## Enums

- `Type`: `Directional`, `Point`, `Spot`, `Area`, `// optional/engine-dependent`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `type` — Field of type `Type` used by systems that consume this component.
- `color` — Color or tint control.
- `intensity` — Field of type `float` used by systems that consume this component.
- `hasTemperature` — Field of type `bool` used by systems that consume this component.
- `temperatureKelvin` — Field of type `float` used by systems that consume this component.
- `range` — Field of type `float` used by systems that consume this component.
- `radius` — Size, reach, or distance tuning.
- `direction` — Field of type `math::Vec3` used by systems that consume this component.
- `spotInnerAngleDeg` — Orientation or angle tuning.
- `spotOuterAngleDeg` — Orientation or angle tuning.
- `castShadows` — Field of type `bool` used by systems that consume this component.
- `shadowResolution` — Field of type `std::uint32_t` used by systems that consume this component.
- `shadowBias` — Field of type `float` used by systems that consume this component.
- `normalBias` — Field of type `float` used by systems that consume this component.
- `shadowDistance` — Size, reach, or distance tuning.
- `volumetricLighting` — Field of type `bool` used by systems that consume this component.
- `volumetricIntensity` — Field of type `float` used by systems that consume this component.
- `affectsLayers` — Layer or filtering control.
- `lightMask` — Layer or filtering control.
- `indirectMultiplier` — Field of type `float` used by systems that consume this component.
- `specularIntensity` — Field of type `float` used by systems that consume this component.
- `diffuseIntensity` — Field of type `float` used by systems that consume this component.
- `hasCookieTexture` — Texture or material binding.
- `cookieTextureId` — Texture or material binding.
- `hasLensFlare` — Field of type `bool` used by systems that consume this component.
- `lensFlareId` — Field of type `std::uint32_t` used by systems that consume this component.

## Example

```cpp
auto& light = registry.emplace<ecs::LightComponent>(entity);
light.type = ecs::LightComponent::Type::Directional;
light.color = {1.0f, 0.95f, 0.85f};
light.intensity = 3.0f;
light.castShadows = true;
```

## Notes

- Use temperature settings when you want a warm/cold light balance without hand-tuning RGB values.
- Keep shadow bias and resolution aligned with the scene scale to reduce acne and shimmering.
