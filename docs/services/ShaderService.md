# ShaderService

## Purpose

- ShaderService provides reusable engine behavior.

## Use when

- Loads shader source, resolves include directives, and builds OpenGL programs by shader key.

## Example

```cpp
graphics::ShaderService shaders;
auto* program = shaders.getOrCreate("graphics/shaders/model");
```

## Notes

- Keep shader paths stable because many components reference them by key instead of by direct file access.
