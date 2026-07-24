#pragma once

// Author: Karl-Johan Bailey
//
// Lightweight frame profiler for debug overlays and timing summaries.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace core {

struct FrameTimingSample final {
  std::string label;
  double lastMs = 0.0;
  double avgMs = 0.0;
  double maxMs = 0.0;
};

struct FrameTimingReport final {
  double totalMs = 0.0;
  double avgTotalMs = 0.0;
  double maxTotalMs = 0.0;
  std::string hottestLabel;
  double hottestMs = 0.0;
  std::vector<FrameTimingSample> samples;
};

class FrameDebugger final {
 public:
  explicit FrameDebugger(std::size_t smoothingWindow = 120) : m_smoothingWindow(std::max<std::size_t>(1, smoothingWindow)) {}

  class Scope final {
   public:
    Scope(FrameDebugger& owner, std::string_view label)
        : m_owner(&owner), m_label(label), m_start(std::chrono::steady_clock::now()) {}

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    Scope(Scope&& other) noexcept
        : m_owner(other.m_owner), m_label(std::move(other.m_label)), m_start(other.m_start) {
      other.m_owner = nullptr;
    }

    Scope& operator=(Scope&& other) noexcept {
      if (this == &other) return *this;
      finish();
      m_owner = other.m_owner;
      m_label = std::move(other.m_label);
      m_start = other.m_start;
      other.m_owner = nullptr;
      return *this;
    }

    ~Scope() { finish(); }

   private:
    void finish() {
      if (!m_owner) return;
      const auto end = std::chrono::steady_clock::now();
      const double ms = std::chrono::duration<double, std::milli>(end - m_start).count();
      m_owner->record(m_label, ms);
      m_owner = nullptr;
    }

    FrameDebugger* m_owner = nullptr;
    std::string m_label;
    std::chrono::steady_clock::time_point m_start{};
  };

  void beginFrame() {
    for (auto& entry : m_entries) {
      entry.currentMs = 0.0;
      entry.touched = false;
    }
  }

  void record(std::string_view label, double ms) {
    for (auto& entry : m_entries) {
      if (entry.label == label) {
        entry.currentMs += ms;
        entry.touched = true;
        return;
      }
    }
    Entry entry;
    entry.label = std::string(label);
    entry.currentMs = ms;
    entry.touched = true;
    m_entries.push_back(std::move(entry));
  }

  Scope scoped(std::string_view label) { return Scope(*this, label); }

  const FrameTimingReport& endFrame() {
    m_report.samples.clear();
    m_report.totalMs = 0.0;
    m_report.hottestLabel.clear();
    m_report.hottestMs = 0.0;

    m_frameCount += 1;
    const double alpha = 1.0 / static_cast<double>(std::min<std::size_t>(m_frameCount, m_smoothingWindow));

    for (auto& entry : m_entries) {
      if (!entry.touched && entry.maxMs <= 0.0 && entry.avgMs <= 0.0) continue;

      const double lastMs = entry.touched ? entry.currentMs : 0.0;
      entry.lastMs = lastMs;
      if (m_frameCount == 1) {
        entry.avgMs = lastMs;
      } else {
        entry.avgMs += (lastMs - entry.avgMs) * alpha;
      }
      entry.maxMs = std::max(entry.maxMs, lastMs);

      m_report.totalMs += lastMs;
      if (lastMs > m_report.hottestMs) {
        m_report.hottestMs = lastMs;
        m_report.hottestLabel = entry.label;
      }

      FrameTimingSample sample;
      sample.label = entry.label;
      sample.lastMs = lastMs;
      sample.avgMs = entry.avgMs;
      sample.maxMs = entry.maxMs;
      m_report.samples.push_back(std::move(sample));
    }

    if (m_frameCount == 1) {
      m_report.avgTotalMs = m_report.totalMs;
    } else {
      m_report.avgTotalMs += (m_report.totalMs - m_report.avgTotalMs) * alpha;
    }
    m_report.maxTotalMs = std::max(m_report.maxTotalMs, m_report.totalMs);
    return m_report;
  }

  const FrameTimingReport& report() const { return m_report; }

 private:
  struct Entry final {
    std::string label;
    double currentMs = 0.0;
    double lastMs = 0.0;
    double avgMs = 0.0;
    double maxMs = 0.0;
    bool touched = false;
  };

  std::size_t m_smoothingWindow = 120;
  std::size_t m_frameCount = 0;
  std::vector<Entry> m_entries;
  FrameTimingReport m_report{};
};

}  // namespace core
