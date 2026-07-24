# IKComponent

## What it is

- IKComponent
- Describes runtime inverse-kinematics chains by bone name.
- The IK system resolves bone names into skeleton indices and updates the skeleton pose.

## When to use it

- Use this when a skeleton needs to reach or plant a bone against a target.

## How it fits

- Treat `IKComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Define the chain and target before running the IK solver each frame.

## Enums

- `TargetMode`: `Entity`, `WorldPosition`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `name` — Stable reference used by content, loaders, or rendering systems.
- `boneNames` — Stable reference used by content, loaders, or rendering systems.
- `targetMode` — Enum or bitmask value that changes system behavior.
- `targetEntity` — Reference to another entity or a linked runtime object.
- `targetEntityName` — Stable reference used by content, loaders, or rendering systems.
- `worldTarget` — Reference to another entity or a linked runtime object.
- `targetOffset` — Spatial placement or directional tuning.
- `targetLocalOffset` — Spatial placement or directional tuning.
- `weight` — Numeric tuning used by gameplay or rendering systems.
- `blendInSeconds` — Field of type `float` consumed by systems that read this component.
- `blendOutSeconds` — Field of type `float` consumed by systems that read this component.
- `currentBlend` — Field of type `float` consumed by systems that read this component.
- `iterations` — Field of type `int` consumed by systems that read this component.
- `overrideAnimation` — Stable reference used by content, loaders, or rendering systems.
- `chains` — Field of type `std::vector<Chain>` consumed by systems that read this component.
- `solvedBoneNames` — Stable reference used by content, loaders, or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::IKComponent>(entity);
component = ecs::IKComponent{};
```

## Common pairings

- Pair `IKComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `IKComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
