#pragma once

// Author: Karl-Johan Bailey
//
// EventService (minimal)
// Stores per-frame events emitted by systems and allows fast "has events of type T?"
// checks so callers can avoid iterating when nothing happened.
//
// Lifetime:
// - Events are intended to live for one simulation cycle.
// - Call `clear()` once per frame at a safe point (typically start of a tick).

#include <cstddef>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ecs::services {

class EventService final {
 public:
  EventService() = default;
  ~EventService() = default;

  EventService(const EventService&) = delete;
  EventService& operator=(const EventService&) = delete;

  void clear() {
    for (auto& [_, bucket] : m_buckets) {
      bucket->clear();
    }
  }

  template <typename T>
  void emit(T e) {
    bucket<T>().events.push_back(std::move(e));
  }

  template <typename T>
  bool has() const {
    auto it = m_buckets.find(std::type_index(typeid(T)));
    if (it == m_buckets.end()) return false;
    return it->second->size() > 0;
  }

  template <typename T>
  std::vector<T> consumeAll() {
    auto& b = bucket<T>();
    std::vector<T> out;
    out.swap(b.events);
    return out;
  }

  template <typename T>
  const std::vector<T>& peekAll() const {
    auto it = m_buckets.find(std::type_index(typeid(T)));
    if (it == m_buckets.end()) {
      static const std::vector<T> empty;
      return empty;
    }
    return static_cast<const Bucket<T>*>(it->second.get())->events;
  }

 private:
  struct IBucket {
    virtual ~IBucket() = default;
    virtual void clear() = 0;
    virtual std::size_t size() const = 0;
  };

  template <typename T>
  struct Bucket final : IBucket {
    std::vector<T> events;
    void clear() override { events.clear(); }
    std::size_t size() const override { return events.size(); }
  };

  template <typename T>
  Bucket<T>& bucket() {
    const std::type_index key(typeid(T));
    auto it = m_buckets.find(key);
    if (it != m_buckets.end()) {
      return *static_cast<Bucket<T>*>(it->second.get());
    }
    auto b = std::make_unique<Bucket<T>>();
    auto* raw = b.get();
    m_buckets.emplace(key, std::move(b));
    return *raw;
  }

  std::unordered_map<std::type_index, std::unique_ptr<IBucket>> m_buckets;
};

}  // namespace ecs::services
