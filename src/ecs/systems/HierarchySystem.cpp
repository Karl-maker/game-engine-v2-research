#include "ecs/systems/HierarchySystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/HierarchyComponent.h"
#include "ecs/components/TransformComponent.h"

#include "math/Mat4.h"

#include <algorithm>

namespace {

static float degToRad(float degrees) { return degrees * 3.14159265358979323846f / 180.0f; }

static math::Mat4 composeLocal(const ecs::HierarchyComponent& h) {
  return math::mul(math::translate(h.localPosition),
                   math::mul(math::rotateY(degToRad(h.localRotation.y)),
                             math::mul(math::rotateX(degToRad(h.localRotation.x)),
                                       math::mul(math::rotateZ(degToRad(h.localRotation.z)), math::scale(h.localScale)))));
}

static math::Mat4 composeWorld(const ecs::TransformComponent& tr) {
  return math::mul(math::translate(tr.position),
                   math::mul(math::rotateY(degToRad(tr.rotation.y)),
                             math::mul(math::rotateX(degToRad(tr.rotation.x)),
                                       math::mul(math::rotateZ(degToRad(tr.rotation.z)), math::scale(tr.scale)))));
}

static math::Vec3 translationFromMat4(const math::Mat4& m) { return {m.m[12], m.m[13], m.m[14]}; }

}  // namespace

namespace ecs::systems {

void HierarchySystem::tick(EntityRegistry& registry) const {
  registry.view<ecs::HierarchyComponent, ecs::TransformComponent>(
      [&](ecs::EntityId id, ecs::HierarchyComponent& hierarchy, ecs::TransformComponent& tr) {
        if (!hierarchy.enabled || hierarchy.parentEntity == ecs::kInvalidEntityId) return;
        if (!registry.isAlive(hierarchy.parentEntity)) return;
        const auto* parentTr = registry.tryGet<ecs::TransformComponent>(hierarchy.parentEntity);
        if (!parentTr) return;

        const math::Mat4 parentWorld = composeWorld(*parentTr);
        const math::Mat4 local = composeLocal(hierarchy);
        const math::Mat4 world = math::mul(parentWorld, local);
        tr.position = translationFromMat4(world);
        if (hierarchy.inheritRotation) tr.rotation = parentTr->rotation + hierarchy.localRotation;
        if (hierarchy.inheritScale) tr.scale = parentTr->scale;

        auto* parentHierarchy = registry.tryGet<ecs::HierarchyComponent>(hierarchy.parentEntity);
        if (parentHierarchy) {
          const bool alreadyListed = std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), id) !=
                                    parentHierarchy->children.end();
          if (!alreadyListed) parentHierarchy->children.push_back(id);
        }
      });
}

}  // namespace ecs::systems
