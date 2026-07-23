# Character Mesh Loading

The mesh path lives in components. The renderer and asset services decide how to load it based on extension.

```cpp
auto character = registry.createEntity("business_man");
auto& tr = registry.emplace<ecs::TransformComponent>(character);
tr.position = {0.0f, 0.0f, -8.0f};

registry.emplace<ecs::ControllerComponent>(character);
registry.emplace<ecs::CharacterComponent>(character);
registry.emplace<ecs::MotionComponent>(character).mode = ecs::MotionComponent::Mode::Walking;

auto& mesh = registry.emplace<ecs::MeshComponent>(character);
mesh.meshId = "business-man";
mesh.meshData.enabled = true;
mesh.meshData.key = "assets/models/business-man/scene.gltf";
mesh.meshType = ecs::MeshComponent::MeshType::Skinned;
mesh.skeletonId = "business-man#skin0";
mesh.castShadows = true;
mesh.receiveShadows = true;

auto& shader = registry.emplace<ecs::ShaderComponent>(character);
shader.shader.key = "graphics/shaders/model";
shader.castShadows = true;
shader.receiveShadows = true;

auto& skeleton = registry.emplace<ecs::SkeletonComponent>(character);
skeleton.skeletonId = "business-man#skin0";
skeleton.skeletonData = "assets/models/business-man/scene.gltf";

auto& animation = registry.emplace<ecs::AnimationComponent>(character);
animation.availableClips = {"IdleV4.2(maya_head)", "Idle", "Walk", "Run"};
animation.layers.push_back({"Base Layer", 1.0f, ecs::AnimationComponent::BlendMode::Override, {}, "Idle", "", 0.0f});
```

The current loader supports `.gltf` and extracts submeshes, materials, textures, skeleton metadata, and animation clip names/durations. New file types should implement `assets::IMeshAssetLoader` and register by extension in `MeshAssetService`.

Third-person camera setup:

```cpp
auto& cameraFollow = registry.emplace<ecs::ThirdPersonCameraComponent>(camera);
cameraFollow.target = character;
cameraFollow.distance = 4.8f;
cameraFollow.height = 1.1f;
```
