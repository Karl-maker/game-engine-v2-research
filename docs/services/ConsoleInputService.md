# ConsoleInputService

## Purpose

- ConsoleInputService provides reusable engine behavior.

## Use when

- Reads terminal input on a background thread and delivers lines to the game loop.

## Example

```cpp
core::ConsoleInputService input;
input.start();
auto lines = input.drainLines();
```

## Notes

- This is the default debug-friendly input path, but it can be swapped for platform input later.
