#pragma once

// Author: Karl-Johan Bailey
//
// Entity Register Service (minimal ECS registry)
// Goals:
// - Efficient component presence checks and iteration by component set
// - O(1) lookup by entity id
// - Type-safe component attach/get/remove
// - Deferred "apply" queue for safe, deterministic edits (no mid-iteration invalidation)
//
// Where systems write:
// - Prefer `defer...` methods during a frame to queue edits.
// - Call `applyDeferred()` at a safe synchronization point (end of tick).

#include "EntityId.h"
#include "components/IdentityComponent.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ecs {

namespace detail {

class IStorage {
 public:
  virtual ~IStorage() = default;
  virtual void remove(EntityId id) = 0;
};

template <typename T>
class SparseSetStorage final : public IStorage {
 public:
  static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

  bool has(EntityId id) const {
    const auto idx = sparseIndex(id);
    return idx != npos;
  }

  T& get(EntityId id) { return denseComponents.at(sparseIndexChecked(id)); }
  const T& get(EntityId id) const { return denseComponents.at(sparseIndexChecked(id)); }

  template <typename... Args>
  T& emplace(EntityId id, Args&&... args) {
    ensureSparseSize(id);
    auto& slot = sparse[static_cast<std::size_t>(id)];
    if (slot != npos) {
      denseComponents[slot] = T{std::forward<Args>(args)...};
      return denseComponents[slot];
    }

    const std::size_t newIndex = denseEntities.size();
    denseEntities.push_back(id);
    denseComponents.emplace_back(std::forward<Args>(args)...);
    slot = newIndex;
    return denseComponents.back();
  }

  void remove(EntityId id) override {
    const auto denseIndex = sparseIndex(id);
    if (denseIndex == npos) {
      return;
    }

    const std::size_t lastIndex = denseEntities.size() - 1;
    if (denseIndex != lastIndex) {
      const EntityId movedId = denseEntities[lastIndex];
      denseEntities[denseIndex] = movedId;
      denseComponents[denseIndex] = std::move(denseComponents[lastIndex]);
      sparse[static_cast<std::size_t>(movedId)] = denseIndex;
    }

    denseEntities.pop_back();
    denseComponents.pop_back();
    sparse[static_cast<std::size_t>(id)] = npos;
  }

  std::size_t size() const { return denseEntities.size(); }
  const std::vector<EntityId>& entities() const { return denseEntities; }

 private:
  void ensureSparseSize(EntityId id) {
    const std::size_t required = static_cast<std::size_t>(id) + 1;
    if (required > sparse.size()) {
      sparse.resize(required, npos);
    }
  }

  std::size_t sparseIndex(EntityId id) const {
    const std::size_t idx = static_cast<std::size_t>(id);
    if (idx >= sparse.size()) {
      return npos;
    }
    return sparse[idx];
  }

  std::size_t sparseIndexChecked(EntityId id) const {
    const auto idx = sparseIndex(id);
    if (idx == npos) {
      throw std::out_of_range("Component not present on entity");
    }
    return idx;
  }

  std::vector<EntityId> denseEntities;
  std::vector<T> denseComponents;
  std::vector<std::size_t> sparse;
};

class CommandQueue final {
 public:
  void enqueue(std::function<void()>&& fn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.emplace_back(std::move(fn));
  }

  void drainAndRun() {
    std::vector<std::function<void()>> local;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      local.swap(m_queue);
    }
    for (auto& fn : local) {
      fn();
    }
  }

 private:
  std::mutex m_mutex;
  std::vector<std::function<void()>> m_queue;
};

template <typename Tuple, std::size_t... Is>
void indexOfSmallestStorageImpl(const Tuple& storages,
                                std::size_t& bestIndex,
                                std::size_t& bestSize,
                                std::index_sequence<Is...>);

template <typename... Storages>
std::size_t indexOfSmallestStorage(const std::tuple<Storages*...>& storages) {
  std::size_t bestIndex = 0;
  std::size_t bestSize = std::numeric_limits<std::size_t>::max();

  constexpr std::size_t N = sizeof...(Storages);
  indexOfSmallestStorageImpl(storages, bestIndex, bestSize, std::make_index_sequence<N>{});

  return bestIndex;
}

template <typename Tuple, std::size_t... Is>
void indexOfSmallestStorageImpl(const Tuple& storages,
                                std::size_t& bestIndex,
                                std::size_t& bestSize,
                                std::index_sequence<Is...>) {
  (void)std::initializer_list<int>{
      ([&] {
        auto* s = std::get<Is>(storages);
        if (!s) return 0;
        const auto sz = s->size();
        if (sz < bestSize) {
          bestSize = sz;
          bestIndex = Is;
        }
        return 0;
      }())...};
}

}  // namespace detail

class EntityRegistry final {
 public:
  EntityRegistry();

  // Creates a new entity and returns its allocated id.
  // - Allocation is monotonic (+1).
  // - An `IdentityComponent` is automatically attached.
  EntityId createEntity(std::string name = {});

  // Destroys an entity and removes all of its components.
  void destroyEntity(EntityId id);

  // Returns true if the id currently refers to a live entity.
  bool isAlive(EntityId id) const;

  // Convenience accessors for the always-present Identity component.
  IdentityComponent& identity(EntityId id);
  const IdentityComponent& identity(EntityId id) const;

  template <typename T, typename... Args>
  // Adds (or replaces) a component on an entity.
  // Example:
  //   registry.emplace<TransformComponent>(id);
  //   registry.emplace<Health>(id, 100);
  T& emplace(EntityId id, Args&&... args) {
    ensureAlive(id);
    auto& storage = storageFor<T>();
    return storage.emplace(id, std::forward<Args>(args)...);
  }

  template <typename T>
  // Checks if an entity has a component of type T.
  bool has(EntityId id) const {
    if (!isAlive(id)) return false;
    const auto* s = tryStorageFor<T>();
    return s ? s->has(id) : false;
  }

  template <typename T>
  // Gets a component reference (throws if missing).
  T& get(EntityId id) {
    ensureAlive(id);
    return storageFor<T>().get(id);
  }

  template <typename T>
  // Gets a component reference (throws if missing).
  const T& get(EntityId id) const {
    ensureAlive(id);
    return storageFor<T>().get(id);
  }

  template <typename T>
  // Gets a component pointer (nullptr if missing).
  T* tryGet(EntityId id) {
    if (!isAlive(id)) return nullptr;
    auto* s = tryStorageFor<T>();
    if (!s || !s->has(id)) return nullptr;
    return &s->get(id);
  }

  template <typename T>
  // Gets a component pointer (nullptr if missing).
  const T* tryGet(EntityId id) const {
    if (!isAlive(id)) return nullptr;
    const auto* s = tryStorageFor<T>();
    if (!s || !s->has(id)) return nullptr;
    return &s->get(id);
  }

  template <typename T>
  // Removes a component (no-op if missing).
  // Example:
  //   registry.remove<TransformComponent>(id);
  void remove(EntityId id) {
    if (!isAlive(id)) return;
    if (auto* s = tryStorageFor<T>()) {
      s->remove(id);
    }
  }

  // Efficient filtering:
  // Iterates entities that have ALL requested components and calls:
  //   fn(EntityId, Components&...)
  template <typename... Components, typename Fn>
  // Iterates entities that have ALL listed components.
  // - Uses the smallest dense set as the primary iterator.
  // - Safe to call `defer...` methods inside the callback; call `applyDeferred()` later.
  void view(Fn&& fn) {
    static_assert(sizeof...(Components) > 0, "view requires at least one component type");

    auto storages = std::make_tuple(tryStorageFor<Components>()...);
    if (!allNonNull(storages)) {
      return;
    }

    const std::size_t primaryIndex = detail::indexOfSmallestStorage(storages);
    visitPrimary(storages, primaryIndex, [&](const auto& primaryEntities) {
      for (const EntityId id : primaryEntities) {
        if (!isAlive(id)) continue;
        if ((std::get<detail::SparseSetStorage<Components>*>(storages)->has(id) && ...)) {
          fn(id, std::get<detail::SparseSetStorage<Components>*>(storages)->get(id)...);
        }
      }
    });
  }

  // Deferred edit API (command buffer).
  // Use these inside systems, then call `applyDeferred()` once per frame.

  // Applies all queued edits in FIFO order.
  void applyDeferred();

  template <typename T, typename... Args>
  // Queues a component add/replace to be applied later.
  void deferEmplace(EntityId id, Args&&... args) {
    m_commands.enqueue([this, id, argsTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
      if (!isAlive(id)) return;
      std::apply(
          [&](auto&&... unpacked) { emplace<T>(id, std::forward<decltype(unpacked)>(unpacked)...); },
          std::move(argsTuple));
    });
  }

  template <typename T>
  // Queues a component removal to be applied later.
  void deferRemove(EntityId id) {
    m_commands.enqueue([this, id] {
      if (!isAlive(id)) return;
      remove<T>(id);
    });
  }

  // Queues an entity destruction to be applied later.
  void deferDestroy(EntityId id) {
    m_commands.enqueue([this, id] {
      if (!isAlive(id)) return;
      destroyEntity(id);
    });
  }

  template <typename T, typename Fn>
  // Queues a mutation lambda to run later if the component exists.
  void deferMutate(EntityId id, Fn&& mutator) {
    m_commands.enqueue([this, id, mutator = std::forward<Fn>(mutator)]() mutable {
      if (!isAlive(id)) return;
      if (auto* c = tryGet<T>(id)) {
        mutator(*c);
      }
    });
  }

  template <typename T, typename MemberT>
  // Queues an arithmetic increment of a specific component member.
  // Example:
  //   registry.deferAdd<TransformComponent>(id, &TransformComponent::position.x, 1.0f);
  void deferAdd(EntityId id, MemberT T::*member, MemberT delta) {
    static_assert(std::is_arithmetic_v<MemberT>, "deferAdd expects an arithmetic member");
    deferMutate<T>(id, [member, delta](T& c) { c.*member += delta; });
  }

 private:
  void ensureAlive(EntityId id) const;

  template <typename T>
  detail::SparseSetStorage<T>& storageFor() const {
    const auto key = std::type_index(typeid(T));
    auto it = m_storages.find(key);
    if (it == m_storages.end()) {
      auto created = std::make_unique<detail::SparseSetStorage<T>>();
      auto* raw = created.get();
      const_cast<EntityRegistry*>(this)->m_storages.emplace(key, std::move(created));
      return *raw;
    }
    return *static_cast<detail::SparseSetStorage<T>*>(it->second.get());
  }

  template <typename T>
  detail::SparseSetStorage<T>* tryStorageFor() {
    const auto key = std::type_index(typeid(T));
    auto it = m_storages.find(key);
    if (it == m_storages.end()) return nullptr;
    return static_cast<detail::SparseSetStorage<T>*>(it->second.get());
  }

  template <typename T>
  const detail::SparseSetStorage<T>* tryStorageFor() const {
    const auto key = std::type_index(typeid(T));
    auto it = m_storages.find(key);
    if (it == m_storages.end()) return nullptr;
    return static_cast<const detail::SparseSetStorage<T>*>(it->second.get());
  }

  template <typename Tuple>
  static bool allNonNull(const Tuple& t) {
    bool ok = true;
    std::apply([&](auto*... ptrs) { ok = ((ptrs != nullptr) && ...); }, t);
    return ok;
  }

  template <typename Tuple, typename Visitor>
  static void visitPrimary(const Tuple& t, std::size_t index, Visitor&& visitor) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    visitPrimaryImpl(t, index, std::forward<Visitor>(visitor), std::make_index_sequence<N>{});
  }

  template <typename Tuple, typename Visitor, std::size_t... Is>
  static void visitPrimaryImpl(const Tuple& t,
                               std::size_t index,
                               Visitor&& visitor,
                               std::index_sequence<Is...>) {
    (void)std::initializer_list<int>{
        ((index == Is) ? (visitor(std::get<Is>(t)->entities()), 0) : 0)...};
  }

  EntityId m_nextId = 1;
  std::vector<std::uint8_t> m_alive;  // index by EntityId (0 unused)
  std::unordered_map<std::type_index, std::unique_ptr<detail::IStorage>> m_storages;
  detail::CommandQueue m_commands;
};

}  // namespace ecs
