# Factories

This folder contains data-driven ECS spawn factories.

## Reusable Inputs

Factories are intended to be composed from small config blocks so you can mix and match without rewriting parsing or setup.

- `TransformInput` (`src/ecs/factories/FactoryInputs.h`)
  - `name`, `position` (or `positionLocal`), `rotationDeg`, `scale`
- `ViewableInput` (`src/ecs/factories/FactoryInputs.h`)
  - Optional visuals: mesh + shader
  - If `meshKey` is empty, no `MeshComponent`/`ShaderComponent` is attached
- `PhysicalInput` (`src/ecs/factories/FactoryInputs.h`)
  - Rigidbody + collider settings (mass/gravity/shape/size/layer)

## Built-in Factory Keys

Factory keys are registered in `src/ecs/factories/FactoryKeyService.cpp` via `registerFactoriesFromEcsFactoriesDir(...)`.

### `actor`

Lightweight world prop: transform + optional visuals. No physics.

Example:
```json
{
  "factory": "actor",
  "name": "wood_01",
  "position": [2.5, 0.0, -7.0],
  "rotationDeg": [0.0, 45.0, 0.0],
  "scale": 1.0,
  "mesh": {
    "key": "assets/models/wood/wood.gltf",
    "type": "static",
    "scale": [1.0, 1.0, 1.0],
    "tags": ["prop"]
  },
  "shader": { "key": "graphics/shaders/model" }
}
```

### `person`

Person/NPC base: transform + physics (rigidbody + collider) + optional visuals.

Example:
```json
{
  "factory": "person",
  "name": "npc_01",
  "position": [0.0, 0.0, 0.0],
  "meshKey": "assets/models/business-man/scene.gltf",
  "meshType": "skinned",
  "skeletonId": "business-man#skin0",
  "skeletonData": "assets/models/business-man/scene.gltf",
  "physical": {
    "mass": 80.0,
    "useGravity": true,
    "shape": "capsule",
    "size": [0.38, 1.85, 0.0],
    "offset": [0.0, 0.925, 0.0],
    "layer": "character"
  }
}
```

## Adding A New Factory

1. Create `XFactory.h/.cpp` that takes a config composed from `TransformInput`/`ViewableInput`/`PhysicalInput` (and any extra inputs you add).
2. Create a JSON adapter implementing `ecs::services::IEntityFactory` (pattern: `ActorJsonFactory` / `PersonJsonFactory`).
3. Register it in `registerFactoriesFromEcsFactoriesDir(...)` in `src/ecs/factories/FactoryKeyService.cpp`.
4. Add the new `.cpp` file to the `duppy` target sources in `CMakeLists.txt`.
