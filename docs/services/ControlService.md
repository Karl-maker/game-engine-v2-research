# ControlService

## Purpose

- ControlService provides reusable engine behavior.

## Use when

- Converts command-style or realtime input into a control state that gameplay systems can consume.

## Example

```cpp
core::ControlService controls;
controls.update(ctx);
auto state = controls.state();
```

## Notes

- This is a good boundary between text commands, controller input, and ECS control components.
