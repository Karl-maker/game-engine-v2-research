# EntityFactoryRegistry

## What it is

- EntityFactoryRegistry
- Maps string keys to owned `IEntityFactory` instances.
- Used by chunk streaming to spawn entities from data.

## When to use it

- Use this when you need a central registry that maps string keys like `character` or `terrain` to actual factory objects.

## Lifecycle

- Build the registry during game setup.
- Register each factory key once.
- Query by key during chunk loading or other data-driven spawn steps.

## What a good registry setup looks like

- All built-in keys are registered before the first chunk loads.
- Factory instances live as long as the registry does.
- The registry is the only place that knows how to translate string keys into factory objects.

## Example

```cpp
ecs::services::EntityFactoryRegistry factories;
factories.registerFactory("character", std::make_unique<MyCharacterFactory>());
auto* factory = factories.find("character");
```

## How it connects

- Stores the factory map used by chunk loading.
- Lets content resolve keys to the correct creator object.

## Notes

- Treat a missing key as content or registration mismatch, not a normal code path.
