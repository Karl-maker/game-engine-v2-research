# IEntityFactory

## What it is

- IEntityFactory
- Data-driven entity factory interface. Factories create ECS entities based on JSON config.

## When to use it

- Implement this when a chunk entry or gameplay system needs to create an entity from data.

## Lifecycle

- A chunk loader or gameplay system owns the config object.
- The loader passes JSON plus a `FactoryContext` into `create(...)`.
- The factory creates the entity, attaches the required components, and returns the new entity id.

## What a good implementation does

- Reads only the data it needs from the JSON object.
- Uses the factory context for chunk-relative placement and other shared spawn metadata.
- Keeps all engine-specific spawning details inside the factory so chunk content stays declarative.

## Example

```cpp
class MyFactory final : public ecs::services::IEntityFactory {
 public:
  ecs::EntityId create(ecs::EntityRegistry& registry,
                       const data::JsonValue& config,
                       const ecs::services::FactoryContext& ctx) override;
};
```

## How it connects

- Integrate `IEntityFactory` where that responsibility belongs in the engine boundary.
- Register the implementation in `EntityFactoryRegistry` so chunk JSON can resolve it by key.

## Notes

- Use the factory context for chunk-relative placement and other streaming metadata.
