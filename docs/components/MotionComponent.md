# MotionComponent

## Purpose

- MotionComponent (lean runtime state)
- Stores current movement state only (what is happening), not gameplay rules (why/how).
- Systems can read/write this during simulation to drive Transform/Rigidbody, animation, etc.

## Use when

- Use this for high-level movement intent, locomotion mode, or velocity tuning that systems convert into motion.

## Enums

- `Mode`: `Walking`, `Running`, `Crouching`, `Sliding`, `Climbing`, `Dashing`, `Jumping`, `Flying`, `Gliding`, `Swimming`, `Falling`, `Physics`, `// driven primarily by a physics simulation`
- `MovementPhase`: `Idle`, `Starting`, `Moving`, `Stopping`

## Key fields

- `mode` — Field of type `Mode` used by systems that consume this component.
- `movementPhase` — Field of type `MovementPhase` used by systems that consume this component.
- `desiredDirection` — Field of type `math::Vec3` used by systems that consume this component.
- `desiredVelocity` — Field of type `math::Vec3` used by systems that consume this component.
- `velocity` — Field of type `math::Vec3` used by systems that consume this component.
- `acceleration` — Field of type `math::Vec3` used by systems that consume this component.
- `currentSpeed` — Movement or force tuning.
- `groundNormal` — Field of type `math::Vec3` used by systems that consume this component.
- `isMoving` — Field of type `bool` used by systems that consume this component.
- `isGrounded` — Field of type `bool` used by systems that consume this component.

## Example

```cpp
auto& motion = registry.emplace<ecs::MotionComponent>(entity);
motion.mode = ecs::MotionComponent::Mode::Walking;
motion.speed = 4.0f;
```

## Notes

- This is a good place to separate player intent from low-level rigidbody integration.
