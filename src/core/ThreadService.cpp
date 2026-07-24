#include "core/ThreadService.h"

// Author: Karl-Johan Bailey

#include <algorithm>

namespace core {

ThreadService::ThreadService(std::size_t workerCount) {
  if (workerCount == 0) {
    const unsigned hc = std::max(1u, std::thread::hardware_concurrency());
    workerCount = (hc > 1u) ? static_cast<std::size_t>(hc - 1u) : 0u;
  }

  m_workers.reserve(workerCount);
  for (std::size_t i = 0; i < workerCount; ++i) {
    m_workers.emplace_back([this] { workerMain(); });
  }
}

ThreadService::~ThreadService() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = true;
  }
  m_cv.notify_all();
  for (auto& t : m_workers) {
    if (t.joinable()) t.join();
  }
}

void ThreadService::submit(std::function<void()> job) {
  if (!job) return;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.emplace(std::move(job));
  }
  m_cv.notify_one();
}

void ThreadService::workerMain() {
  while (true) {
    std::function<void()> job;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [&] { return m_stop || !m_jobs.empty(); });
      if (m_stop && m_jobs.empty()) return;
      job = std::move(m_jobs.front());
      m_jobs.pop();
    }
    job();
  }
}

}  // namespace core

