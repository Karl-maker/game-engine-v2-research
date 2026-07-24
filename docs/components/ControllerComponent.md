# ControllerComponent

## What it is

- ControllerComponent (request interface)
- This component receives control requests from a control service (player input, AI, network, scripts).
- It is intentionally descriptive and "request-only" — movement/aim/shoot systems interpret requests.

## When to use it

- Use this when an entity should respond to input, AI control, or scripted action requests.

## How it fits

- Treat `ControllerComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Write the intent here, then let movement, jump, and combat systems consume it.

## Enums

- `Mode`: `Player`, `AI`, `Network`, `Script`
- `MoveMode`: `Walk`, `Sprint`, `Crouch`

## Field guide

- `enabled` — Master on/off switch or similar behavior flag.
- `mode` — Enum or bitmask value that changes system behavior.
- `hasRequest` — Field of type `bool` consumed by systems that read this component.
- `hasDirection` — Spatial placement or directional tuning.
- `direction` — Spatial placement or directional tuning.
- `moveMode` — Enum or bitmask value that changes system behavior.
- `hasDestination` — Field of type `bool` consumed by systems that read this component.
- `destination` — Field of type `math::Vec3` consumed by systems that read this component.
- `acceptanceRadius` — Size, reach, or distance tuning.
- `moveRequest` — Field of type `}` consumed by systems that read this component.
- `lookDelta` — Field of type `math::Vec3` consumed by systems that read this component.
- `lookDirection` — Spatial placement or directional tuning.
- `lookRequest` — Field of type `}` consumed by systems that read this component.
- `action` — Field of type `std::string` consumed by systems that read this component.
- `pressed` — Field of type `bool` consumed by systems that read this component.
- `value` — Numeric tuning used by gameplay or rendering systems.
- `actionRequests` — Field of type `std::vector<ActionRequest>` consumed by systems that read this component.
- `priority` — Field of type `int` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::ControllerComponent>(entity);
component = ecs::ControllerComponent{};
```

## Common pairings

- Pair `ControllerComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `ControllerComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
