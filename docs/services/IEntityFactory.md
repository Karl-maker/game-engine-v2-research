# IEntityFactory

## Purpose

- IEntityFactory provides reusable engine behavior.

## Use when

- Factory interface that turns JSON config into live ECS entities.

## Example

```cpp
class MyFactory final : public ecs::services::IEntityFactory {
 public:
  ecs::EntityId create(ecs::EntityRegistry& registry,
                       const data::JsonValue& config,
                       const ecs::services::FactoryContext& ctx) override;
};
```

## Notes

- Use the factory context when you need chunk-relative placement or other streaming metadata.
