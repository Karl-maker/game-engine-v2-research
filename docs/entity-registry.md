# Entity Registry (ECS) Usage

Author: Karl-Johan Bailey

This project includes a minimal Entity Registry that:
- Allocates monotonic `EntityId` values (+1)
- Auto-attaches `IdentityComponent` (`id`, `name`)
- Lets you add/remove components type-safely
- Supports fast filtering with `view<A, B, ...>()`
- Provides a deferred command buffer (`defer...` + `applyDeferred()`) to avoid mid-iteration edits

## Includes

```cpp
#include "ecs/EntityRegistry.h"
#include "ecs/components/TransformComponent.h"
```

## 1) Create (add) an entity

```cpp
ecs::EntityRegistry registry;

ecs::EntityId player = registry.createEntity("Player"); // allocates id + adds IdentityComponent
registry.emplace<ecs::TransformComponent>(player);      // add TransformComponent
```

## 2) Find an entity by id

```cpp
if (!registry.isAlive(player)) {
  // entity was destroyed (or id is invalid)
  return;
}

// IdentityComponent is always present on live entities created by createEntity().
const auto& ident = registry.identity(player);
// ident.id, ident.name

// Other components:
if (auto* tr = registry.tryGet<ecs::TransformComponent>(player)) {
  // found (pointer is non-null)
  (void)tr;
}
```

## Method cookbook (example for every public method)

Assume:
```cpp
ecs::EntityRegistry registry; // EntityRegistry()
ecs::EntityId id = registry.createEntity("Crate");
```

### `createEntity(name)`
```cpp
ecs::EntityId crate = registry.createEntity("Crate");
```

### `destroyEntity(id)`
```cpp
registry.destroyEntity(id);
```

### `isAlive(id)`
```cpp
if (registry.isAlive(id)) {
  // ok to access components
}
```

### `identity(id)` (get IdentityComponent)
```cpp
auto& ident = registry.identity(id);      // mutable
const auto& ci = registry.identity(id);   // const
ident.name = "New Name";
```

### `emplace<T>(id, ...)` (add/replace a component)
```cpp
registry.emplace<ecs::TransformComponent>(id); // add default transform

// Replaces existing component if already present.
registry.emplace<ecs::TransformComponent>(id);
```

### `has<T>(id)` (check component presence)
```cpp
if (registry.has<ecs::TransformComponent>(id)) {
  // entity has transform
}
```

### `get<T>(id)` (get component reference; throws if missing)
```cpp
auto& tr = registry.get<ecs::TransformComponent>(id);
tr.position.x = 10.0f;
```

### `tryGet<T>(id)` (get component pointer; nullptr if missing)
```cpp
if (auto* tr = registry.tryGet<ecs::TransformComponent>(id)) {
  tr->rotation.yaw += 5.0f;
}
```

### `remove<T>(id)` (remove component; no-op if missing)
```cpp
registry.remove<ecs::TransformComponent>(id);
```

### `view<A, B, ...>(fn)` (filter entities by component set)
```cpp
registry.view<ecs::IdentityComponent, ecs::TransformComponent>(
    [](ecs::EntityId entity, ecs::IdentityComponent& ident, ecs::TransformComponent& tr) {
      (void)entity;
      (void)ident;
      tr.position.z += 0.1f;
    });
```

### `applyDeferred()` (run queued edits)
```cpp
registry.applyDeferred(); // call once at a safe sync point (e.g., end of frame)
```

### `deferEmplace<T>(id, ...)` (queue add/replace component)
```cpp
registry.deferEmplace<ecs::TransformComponent>(id); // queued add
registry.applyDeferred();
```

### `deferRemove<T>(id)` (queue remove component)
```cpp
registry.deferRemove<ecs::TransformComponent>(id);
registry.applyDeferred();
```

### `deferDestroy(id)` (queue entity destruction)
```cpp
registry.deferDestroy(id);
registry.applyDeferred();
```

### `deferMutate<T>(id, mutator)` (queue a mutation)
```cpp
registry.deferMutate<ecs::IdentityComponent>(id, [](ecs::IdentityComponent& ident) {
  ident.name = "Queued Rename";
});
registry.applyDeferred();
```

### `deferAdd<T>(id, &T::member, delta)` (queue a numeric increment)
```cpp
registry.deferAdd<ecs::TransformComponent>(id, &ecs::TransformComponent::position.x, 1.0f);
registry.applyDeferred();
```

## 3) Edit an entity (immediate)

```cpp
auto& tr = registry.get<ecs::TransformComponent>(player); // throws if missing
tr.position.x += 1.0f;
tr.rotation.yaw += 15.0f;
```

## 4) Edit an entity (deferred / queued) and apply safely

Use this when many systems might edit the same entity during a tick, or when you’re iterating views and don’t want to invalidate anything mid-loop.

```cpp
// Queue a safe mutation (runs later if the component exists).
registry.deferMutate<ecs::TransformComponent>(player, [](ecs::TransformComponent& tr) {
  tr.position.y += 2.0f;
});

// Queue a typed “increment” of a specific member.
registry.deferAdd<ecs::TransformComponent>(player, &ecs::TransformComponent::position.x, 0.5f);

// Apply all queued edits at a sync point (typically end of frame).
registry.applyDeferred();
```

## 5) Filter entities that have specific components

```cpp
registry.view<ecs::IdentityComponent, ecs::TransformComponent>(
    [](ecs::EntityId id, ecs::IdentityComponent& ident, ecs::TransformComponent& tr) {
      (void)id;
      // Update simulation, read/modify components, etc.
      tr.position.z += 0.1f;
      ident.name = ident.name; // example access
    });
```

Tip: if you need to add/remove components or destroy entities while iterating a `view`, prefer `deferEmplace`, `deferRemove`, and `deferDestroy`, then call `applyDeferred()` once after the view.

## 6) Remove components and destroy entities

```cpp
registry.remove<ecs::TransformComponent>(player); // remove component (no-op if missing)

registry.destroyEntity(player);                   // removes all components and marks id not alive
// registry.isAlive(player) == false
```
