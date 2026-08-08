// Author: Karl-Johan Bailey

#include "ecs/systems/InteractionDetectionSystem.h"

#include "ecs/components/ControllerComponent.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/InteractiveComponent.h"
#include "ecs/components/BillboardComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/events/RaycastEvents.h"

namespace {

const ecs::IdentityComponent* identityOrNull(ecs::EntityRegistry& registry, ecs::EntityId entity) {
  if (entity == ecs::kInvalidEntityId || !registry.isAlive(entity)) return nullptr;
  return registry.tryGet<ecs::IdentityComponent>(entity);
}

std::string nameForEntity(ecs::EntityRegistry& registry, ecs::EntityId entity) {
  if (const auto* identity = identityOrNull(registry, entity)) return identity->name;
  return {};
}

bool isPlayerSensorSource(ecs::EntityRegistry& registry, ecs::EntityId parentEntity) {
  if (parentEntity == ecs::kInvalidEntityId || !registry.isAlive(parentEntity)) return false;
  return registry.tryGet<ecs::ControllerComponent>(parentEntity) != nullptr;
}

// Helper to create or update billboard for interactive entity
void updateInteractiveBillboard(ecs::EntityRegistry& registry, ecs::EntityId entity, bool show, float distance) {
  if (entity == ecs::kInvalidEntityId || !registry.isAlive(entity)) return;
  
  // Check if the entity already has a billboard
  auto* billboard = registry.tryGet<ecs::BillboardComponent>(entity);
  
  if (show) {
    if (!billboard) {
      // Create new billboard component
      auto& newBillboard = registry.emplace<ecs::BillboardComponent>(entity);
      newBillboard.enabled = true;
      newBillboard.visible = true;
      newBillboard.textureEnabled = true;
      newBillboard.depthWrite = false;
      newBillboard.doubleSided = true;
      newBillboard.faceMode = ecs::BillboardComponent::FaceMode::CameraPlane;
      newBillboard.sizeMeters = {0.2f, 0.2f}; // Adjust size as needed
      newBillboard.pivot = {0.5f, 1.0f}; // Pivot at bottom-center
      newBillboard.worldOffset = {0.0f, 1.5f, 0.0f}; // Offset above the object
      newBillboard.maxRenderDistance = 96.0f;
      newBillboard.texture.key = "assets/hud/e-key.png";
      newBillboard.texture.id = 0; // Will be loaded by asset manager
      newBillboard.tint = {1.0f, 1.0f, 1.0f, 1.0f};
      
      // Ensure TransformComponent exists
      if (!registry.tryGet<ecs::TransformComponent>(entity)) {
        registry.emplace<ecs::TransformComponent>(entity);
      }
    } else {
      // Update existing billboard
      billboard->enabled = true;
      billboard->visible = true;
      billboard->texture.key = "assets/hud/e-key.png";
    }
  } else {
    // Hide the billboard
    if (billboard) {
      billboard->enabled = false;
      billboard->visible = false;
    }
  }
}

}  // namespace

namespace ecs::systems {

void InteractionDetectionSystem::tick(ecs::EntityRegistry& registry, ecs::services::EventService& events, double timeSeconds) const {
  (void)timeSeconds;

  // Reset the derived interaction source state each frame.
  registry.view<ecs::InteractiveComponent>([](ecs::EntityId, ecs::InteractiveComponent& interactive) {
    interactive.detected_this_frame = false;
    interactive.detected_by_player_sensor = false;
    interactive.source_sensor_entity = ecs::kInvalidEntityId;
    interactive.source_parent_entity = ecs::kInvalidEntityId;
    interactive.source_ray_entity = ecs::kInvalidEntityId;
    interactive.source_distance = 0.0f;
    interactive.source_sensor_name.clear();
    interactive.source_parent_name.clear();
    interactive.source_raycast_category.clear();
  });

  const auto& alerts = events.peekAll<ecs::events::SensorAlertEvent>();
  if (alerts.empty()) return;

  // First pass: find all interactive entities that were detected this frame
  std::vector<ecs::EntityId> detectedEntities;
  
  for (const auto& alert : alerts) {
    auto* interactive = registry.tryGet<ecs::InteractiveComponent>(alert.hitEntity);
    if (!interactive) continue;
    if (alert.hit.distance > interactive->interactive_distance) continue;

    const bool isFirstMatch = !interactive->detected_this_frame;
    const bool isCloserMatch = alert.hit.distance < interactive->source_distance;
    if (!isFirstMatch && !isCloserMatch) continue;

    interactive->detected_this_frame = true;
    interactive->detected_by_player_sensor = isPlayerSensorSource(registry, alert.parentEntity);
    interactive->source_sensor_entity = alert.sensorEntity;
    interactive->source_parent_entity = alert.parentEntity;
    interactive->source_ray_entity = alert.rayEntity;
    interactive->source_distance = alert.hit.distance;
    interactive->source_sensor_name = nameForEntity(registry, alert.sensorEntity);
    interactive->source_parent_name = nameForEntity(registry, alert.parentEntity);
    interactive->source_raycast_category = alert.raycastCategory;

    // Add to detected entities list
    if (std::find(detectedEntities.begin(), detectedEntities.end(), alert.hitEntity) == detectedEntities.end()) {
      detectedEntities.push_back(alert.hitEntity);
    }
  }

  // Second pass: Update billboards for all interactive entities
  registry.view<ecs::InteractiveComponent>([&](ecs::EntityId entity, const ecs::InteractiveComponent& interactive) {
    // Check if this entity was detected this frame and by a player sensor
    bool shouldShowBillboard = interactive.detected_this_frame && interactive.detected_by_player_sensor;
    
    // Update the billboard
    updateInteractiveBillboard(registry, entity, shouldShowBillboard, interactive.source_distance);
  });
}

}  // namespace ecs::systems