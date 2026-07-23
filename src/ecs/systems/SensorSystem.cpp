#include "ecs/systems/SensorSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/SensorComponent.h"
#include "ecs/events/RaycastEvents.h"

namespace ecs::systems {

void SensorSystem::tick(EntityRegistry& registry, ecs::services::EventService& events) const {
  const auto hits = events.consumeAll<ecs::events::RayHitEvent>();

  for (const auto& hit : hits) {
    if (!registry.isAlive(hit.sensorEntity)) continue;
    const auto* sensor = registry.tryGet<ecs::SensorComponent>(hit.sensorEntity);
    if (!sensor || !sensor->enabled) continue;
    if (sensor->parentEntity == ecs::kInvalidEntityId) continue;

    events.emit<ecs::events::SensorAlertEvent>(
        {hit.sensorEntity, sensor->parentEntity, hit.rayEntity, hit.hitEntity, hit.hit, hit.raycastCategory});
  }
}

}  // namespace ecs::systems
