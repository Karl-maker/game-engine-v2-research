# GameLoopService

## Purpose

- GameLoopService provides reusable engine behavior.

## Use when

- Owns the main tick loop, frame timing, input polling, and debug output.

## Example

```cpp
core::GameLoopConfig cfg;
cfg.targetFps = 144.0;
cfg.capFrameRate = true;
```

## Notes

- Put simulation and rendering orchestration behind `core::IGame` so the loop itself stays generic.
