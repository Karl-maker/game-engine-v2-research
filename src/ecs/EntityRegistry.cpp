#include "EntityRegistry.h"

// Author: Karl-Johan Bailey

namespace ecs {

EntityRegistry::EntityRegistry() {
  m_alive.resize(1, 0);
}

EntityId EntityRegistry::createEntity(std::string name) {
  const EntityId id = m_nextId++;
  if (static_cast<std::size_t>(id) >= m_alive.size()) {
    m_alive.resize(static_cast<std::size_t>(id) + 1, 0);
  }
  m_alive[static_cast<std::size_t>(id)] = 1;

  auto& ident = emplace<IdentityComponent>(id);
  ident.id = id;
  ident.name = std::move(name);

  return id;
}

void EntityRegistry::destroyEntity(EntityId id) {
  if (!isAlive(id)) {
    return;
  }

  m_alive[static_cast<std::size_t>(id)] = 0;

  for (auto& [_, storage] : m_storages) {
    storage->remove(id);
  }
}

bool EntityRegistry::isAlive(EntityId id) const {
  const auto idx = static_cast<std::size_t>(id);
  return id != kInvalidEntityId && idx < m_alive.size() && m_alive[idx] != 0;
}

IdentityComponent& EntityRegistry::identity(EntityId id) { return get<IdentityComponent>(id); }
const IdentityComponent& EntityRegistry::identity(EntityId id) const { return get<IdentityComponent>(id); }

void EntityRegistry::applyDeferred() { m_commands.drainAndRun(); }

void EntityRegistry::ensureAlive(EntityId id) const {
  if (!isAlive(id)) {
    throw std::out_of_range("EntityId is not alive");
  }
}

}  // namespace ecs

