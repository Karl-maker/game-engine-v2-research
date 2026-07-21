#pragma once

// Author: Karl-Johan Bailey
//
// Monotonic time source (steady clock):
// - Suitable for delta time / frame timing (won't jump with system clock changes).
// - Returns seconds since construction.

#include "ITimeSource.h"

namespace core {

class SteadyTimeSource final : public ITimeSource {
 public:
  SteadyTimeSource();
  double nowSeconds() const override;

 private:
  double m_startSeconds = 0.0;
};

}  // namespace core
