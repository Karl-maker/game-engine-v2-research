#pragma once

// Author: Karl-Johan Bailey
//
// EntityFactoryRegistry
// - Maps string keys to IEntityFactory instances (owned).
// - Used by chunk streaming to spawn entities from data.

#include "ecs/services/IEntityFactory.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ecs::services {

class EntityFactoryRegistry final {
 public:
  void registerFactory(std::string key, std::unique_ptr<IEntityFactory> factory) {
    if (!factory) return;
    m_factories.emplace(std::move(key), std::move(factory));
  }

  IEntityFactory* find(std::string_view key) const {
    auto it = m_factories.find(std::string(key));
    if (it == m_factories.end()) return nullptr;
    return it->second.get();
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<IEntityFactory>> m_factories;
};

}  // namespace ecs::services

