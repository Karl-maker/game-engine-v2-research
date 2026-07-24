# ConsoleInputService

## What it is

- Console input implementation:
- - Spins a background thread that blocks on `std::getline(std::cin, ...)`.
- - Main loop stays non-blocking and "drains" any lines typed since last frame.
- - This is a simple starting point; swap it for SDL, platform events, etc. later
- by implementing `core::IInputService`.

## When to use it

- Use this as the default debug-friendly input path until you swap in a platform-specific input layer.

## Lifecycle

- Identify which part of the engine owns the work, then start or construct the service in that layer.
- Feed the service the data it needs each frame, tick, or load step.
- Let the service manage the reusable policy instead of repeating that policy in every gameplay system.

## Example

```cpp
core::ConsoleInputService input;
input.start();
auto lines = input.drainLines();
```

## How it connects

- Integrate `ConsoleInputService` where that responsibility belongs in the engine boundary.

## Notes

- It is intentionally simple so the game can be controlled while iterating in the terminal.
