# MotionComponent

## What it is

- MotionComponent (lean runtime state)
- Stores current movement state only (what is happening), not gameplay rules (why/how).
- Systems can read/write this during simulation to drive Transform/Rigidbody, animation, etc.

## When to use it

- Use this when you want a high-level movement state like walking, sprinting, or idle motion.

## How it fits

- Treat `MotionComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Treat it as the bridge between control input and physics-driven movement.

## Enums

- `Mode`: `Walking`, `Running`, `Crouching`, `Sliding`, `Climbing`, `Dashing`, `Jumping`, `Flying`, `Gliding`, `Swimming`, `Falling`, `Physics`, `// driven primarily by a physics simulation`
- `MovementPhase`: `Idle`, `Starting`, `Moving`, `Stopping`

## Field guide

- `mode` — Enum or bitmask value that changes system behavior.
- `movementPhase` — Field of type `MovementPhase` consumed by systems that read this component.
- `desiredDirection` — Spatial placement or directional tuning.
- `desiredVelocity` — Field of type `math::Vec3` consumed by systems that read this component.
- `velocity` — Field of type `math::Vec3` consumed by systems that read this component.
- `acceleration` — Field of type `math::Vec3` consumed by systems that read this component.
- `currentSpeed` — Numeric tuning used by gameplay or rendering systems.
- `groundNormal` — Field of type `math::Vec3` consumed by systems that read this component.
- `isMoving` — Field of type `bool` consumed by systems that read this component.
- `isGrounded` — Field of type `bool` consumed by systems that read this component.

## Example

```cpp
auto& component = registry.emplace<ecs::MotionComponent>(entity);
component = ecs::MotionComponent{};
```

## Common pairings

- Pair `MotionComponent` with the system that owns the simulation or rendering work for this data.
- If the component needs to be created from content, add a factory or chunk entry for it.

## Notes

- Keep `MotionComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
