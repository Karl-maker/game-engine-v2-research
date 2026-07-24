#pragma once

// Author: Karl-Johan Bailey
//
// ThreadService (minimal thread pool)
// Provides a safe place to run heavy compute in parallel. Callers should avoid
// mutating shared ECS state from worker threads; instead compute into local
// outputs and apply on the main thread.

#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace core {

class ThreadService final {
 public:
  explicit ThreadService(std::size_t workerCount = 0);
  ~ThreadService();

  ThreadService(const ThreadService&) = delete;
  ThreadService& operator=(const ThreadService&) = delete;

  std::size_t workerCount() const { return m_workers.size(); }

  void submit(std::function<void()> job);

  template <typename Fn>
  void parallelFor(std::size_t count, std::size_t grain, Fn&& fn) {
    if (count == 0) return;
    if (workerCount() == 0 || count <= grain) {
      for (std::size_t i = 0; i < count; ++i) fn(i);
      return;
    }

    const std::size_t g = (grain == 0) ? 1 : grain;
    auto next = std::make_shared<std::atomic<std::size_t>>(0);
    auto remaining = std::make_shared<std::atomic<int>>(static_cast<int>(workerCount()));
    auto done = std::make_shared<std::atomic<bool>>(false);
    auto mtx = std::make_shared<std::mutex>();
    auto cv = std::make_shared<std::condition_variable>();

    auto workerLoop = [next, remaining, done, mtx, cv, count, g, fn = std::forward<Fn>(fn)]() mutable {
      while (true) {
        const std::size_t start = next->fetch_add(g, std::memory_order_relaxed);
        if (start >= count) break;
        const std::size_t end = std::min(count, start + g);
        for (std::size_t i = start; i < end; ++i) fn(i);
      }

      if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
        {
          std::lock_guard<std::mutex> lock(*mtx);
          done->store(true, std::memory_order_release);
        }
        cv->notify_one();
      }
    };

    for (std::size_t i = 0; i < workerCount(); ++i) {
      submit(workerLoop);
    }

    std::unique_lock<std::mutex> lock(*mtx);
    cv->wait(lock, [&] { return done->load(std::memory_order_acquire); });
  }

 private:
  void workerMain();

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::queue<std::function<void()>> m_jobs;
  std::vector<std::thread> m_workers;
  bool m_stop = false;
};

}  // namespace core
