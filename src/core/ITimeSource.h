#pragma once

namespace core {

class ITimeSource {
 public:
  virtual ~ITimeSource() = default;
  virtual double nowSeconds() const = 0;
};

}  // namespace core

