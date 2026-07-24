# EntityFactoryRegistry

## Purpose

- EntityFactoryRegistry provides reusable engine behavior.

## Use when

- Owns the string-key-to-factory map used by chunk loading and other data-driven spawn systems.

## Example

```cpp
ecs::services::EntityFactoryRegistry factories;
factories.registerFactory("character", std::make_unique<MyCharacterFactory>());
auto* factory = factories.find("character");
```

## Notes

- A missing key should be treated as a content error, not a crash path.
