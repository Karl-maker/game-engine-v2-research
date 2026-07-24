# ControllerComponent

## Purpose

- ControllerComponent (request interface)
- This component receives control requests from a control service (player input, AI, network, scripts).
- It is intentionally descriptive and "request-only" — movement/aim/shoot systems interpret requests.

## Use when

- Use `ControllerComponent` when you need ECS data for controllercomponent behavior.

## Enums

- `Mode`: `Player`, `AI`, `Network`, `Script`
- `MoveMode`: `Walk`, `Sprint`, `Crouch`

## Key fields

- `enabled` — Enable or disable this part of the component.
- `mode` — Field of type `Mode` used by systems that consume this component.
- `hasRequest` — Field of type `bool` used by systems that consume this component.
- `hasDirection` — Field of type `bool` used by systems that consume this component.
- `direction` — Field of type `math::Vec3` used by systems that consume this component.
- `moveMode` — Field of type `MoveMode` used by systems that consume this component.
- `hasDestination` — Field of type `bool` used by systems that consume this component.
- `destination` — Field of type `math::Vec3` used by systems that consume this component.
- `acceptanceRadius` — Size, reach, or distance tuning.
- `moveRequest` — Field of type `}` used by systems that consume this component.
- `lookDelta` — Field of type `math::Vec3` used by systems that consume this component.
- `lookDirection` — Field of type `math::Vec3` used by systems that consume this component.
- `lookRequest` — Field of type `}` used by systems that consume this component.
- `action` — Field of type `std::string` used by systems that consume this component.
- `pressed` — Field of type `bool` used by systems that consume this component.
- `value` — Numeric value used by a system or widget.
- `actionRequests` — Field of type `std::vector<ActionRequest>` used by systems that consume this component.
- `priority` — Field of type `int` used by systems that consume this component.

## Example

```cpp
auto& component = registry.emplace<ecs::ControllerComponent>(entity);
component = ecs::ControllerComponent{};
```

## Notes

- Keep `ControllerComponent` focused on data so systems can stay deterministic and easy to extend.
- Add a system or factory that reads this component instead of putting behavior into the component itself.
