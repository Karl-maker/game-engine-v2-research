#include "SteadyTimeSource.h"

#include <chrono>

namespace core {

static double steadyNowSeconds() {
  using Clock = std::chrono::steady_clock;
  const auto now = Clock::now().time_since_epoch();
  return std::chrono::duration<double>(now).count();
}

SteadyTimeSource::SteadyTimeSource() : m_startSeconds(steadyNowSeconds()) {}

double SteadyTimeSource::nowSeconds() const { return steadyNowSeconds() - m_startSeconds; }

}  // namespace core

