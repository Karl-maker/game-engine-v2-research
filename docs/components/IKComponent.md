# IKComponent

## Purpose

- IKComponent
- Describes runtime inverse-kinematics chains by bone name.
- The IK system resolves bone names into skeleton indices and updates the skeleton pose.

## Use when

- Use `IKComponent` when you need ECS data for ikcomponent behavior.

## Enums

- `TargetMode`: `Entity`, `WorldPosition`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `name` — A stable content or debug identifier.
- `boneNames` — Field of type `std::vector<std::string>` used by systems that consume this component.
- `targetMode` — Reference to another entity or input source.
- `targetEntity` — Reference to another entity or input source.
- `targetEntityName` — Reference to another entity or input source.
- `worldTarget` — Reference to another entity or input source.
- `targetOffset` — Spatial placement or relative offset.
- `targetLocalOffset` — Spatial placement or relative offset.
- `weight` — Field of type `float` used by systems that consume this component.
- `blendInSeconds` — Field of type `float` used by systems that consume this component.
- `blendOutSeconds` — Field of type `float` used by systems that consume this component.
- `currentBlend` — Field of type `float` used by systems that consume this component.
- `iterations` — Field of type `int` used by systems that consume this component.
- `overrideAnimation` — Field of type `bool` used by systems that consume this component.
- `chains` — Field of type `std::vector<Chain>` used by systems that consume this component.
- `solvedBoneNames` — Field of type `std::vector<std::string>` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::IKComponent>(entity);
component = ecs::IKComponent{};
```

## Notes

- Keep `IKComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
