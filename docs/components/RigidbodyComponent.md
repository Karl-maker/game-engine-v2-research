# RigidbodyComponent

## What it is

- RigidbodyComponent (descriptive only)
- Stores physics body properties. A physics system is responsible for integrating velocities
- and applying the result back to TransformComponent.

## When to use it

- Use this when an entity should be simulated as a body with inertia and force response.

## How it fits

- Treat `RigidbodyComponent` as data only: create it in a factory or gameplay setup, then let systems read it later.
- Keep behavior out of the component so the same data can be saved, streamed, or rebuilt from JSON.

## Typical setup

- Set the mass before anything else so inverse-mass calculations stay consistent.
- Decide early whether the body should use gravity or be kinematic.

## Field guide

- `mass` — Numeric tuning used by gameplay or rendering systems.
- `inverseMass` — Numeric tuning used by gameplay or rendering systems.
- `linearVelocity` — Field of type `math::Vec3` consumed by systems that read this component.
- `angularVelocity` — Field of type `math::Vec3` consumed by systems that read this component.
- `linearDamping` — Field of type `float` consumed by systems that read this component.
- `angularDamping` — Field of type `float` consumed by systems that read this component.
- `useGravity` — Master on/off switch or similar behavior flag.
- `kinematic` — Field of type `bool` consumed by systems that read this component.
- `freezePositionX` — Spatial placement or directional tuning.
- `freezePositionY` — Spatial placement or directional tuning.
- `freezePositionZ` — Spatial placement or directional tuning.
- `freezeRotationX` — Orientation or angular tuning.
- `freezeRotationY` — Orientation or angular tuning.
- `freezeRotationZ` — Orientation or angular tuning.
- `sleepEnabled` — Field of type `bool` consumed by systems that read this component.
- `centerOfMass` — Numeric tuning used by gameplay or rendering systems.

## Example

```cpp
auto& component = registry.emplace<ecs::RigidbodyComponent>(entity);
component = ecs::RigidbodyComponent{};
```

## Common pairings

- Pair this with `ColliderComponent` so the body has a shape to collide against.

## Notes

- Keep `RigidbodyComponent` focused on data so systems stay deterministic and easy to extend.
- Update the docs and the matching system together whenever you add a new field.
