# IKService

## Purpose

- IKService provides reusable engine behavior.

## Use when

- Provides helper routines for inverse-kinematics setup and chain manipulation.

## Example

```cpp
ecs::services::IKService ik;
// configure chains, targets, or helper state before the IK system runs.
```

## Notes

- Keep solver setup here so animation systems can stay focused on pose evaluation.
