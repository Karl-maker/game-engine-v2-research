# GameLoopService

## What it is

- Game Loop Service
- - Owns the main "tick" loop and timing.
- - Pulls input from an injected input service.
- - Calls an injected game instance each frame (dependency inversion).
- - Optionally prints a tiny debug HUD (configurable).
- Where game logic goes:
- - Implement `core::IGame` and put gameplay/simulation updates in `onTick(...)`.
- - Read inputs from `TickContext::inputLines` (or swap in a different input service later).
- - Call `TickContext::requestQuit()` to exit the loop.

## When to use it

- Use this when you want the engine to manage frame pacing and call a game object once per tick.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
core::GameLoopConfig cfg;
cfg.targetFps = 144.0;
cfg.capFrameRate = true;
```

## How it connects

- Calls the game implementation once per frame.
- Owns pacing, input drain, and quit handling.

## Notes

- Keep game logic inside `core::IGame` so the loop stays reusable.
