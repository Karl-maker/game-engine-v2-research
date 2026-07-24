# RigidbodyComponent

## Purpose

- RigidbodyComponent (descriptive only)
- Stores physics body properties. A physics system is responsible for integrating velocities
- and applying the result back to TransformComponent.

## Use when

- Use this when an entity should be simulated as a physical body with mass and velocity.

## Key fields

- `mass` — Field of type `float` used by systems that consume this component.
- `inverseMass` — Field of type `float` used by systems that consume this component.
- `linearVelocity` — Field of type `math::Vec3` used by systems that consume this component.
- `angularVelocity` — Field of type `math::Vec3` used by systems that consume this component.
- `linearDamping` — Field of type `float` used by systems that consume this component.
- `angularDamping` — Field of type `float` used by systems that consume this component.
- `useGravity` — Movement or force tuning.
- `kinematic` — Field of type `bool` used by systems that consume this component.
- `freezePositionX` — Spatial placement or relative offset.
- `freezePositionY` — Spatial placement or relative offset.
- `freezePositionZ` — Spatial placement or relative offset.
- `freezeRotationX` — Orientation or angle tuning.
- `freezeRotationY` — Orientation or angle tuning.
- `freezeRotationZ` — Orientation or angle tuning.
- `sleepEnabled` — Enable or disable this part of the component.
- `centerOfMass` — Field of type `math::Vec3` used by systems that consume this component.

## Example

```cpp
auto& body = registry.emplace<ecs::RigidbodyComponent>(entity);
body.mass = 22.0f;
body.useGravity = true;
body.kinematic = false;
```

## Notes

- Keep mass and inverse mass consistent if you override values manually.
