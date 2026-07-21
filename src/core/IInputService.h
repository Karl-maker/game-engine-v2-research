#pragma once

#include <string>
#include <vector>

namespace core {

class IInputService {
 public:
  virtual ~IInputService() = default;
  virtual void start() = 0;
  virtual std::vector<std::string> drainLines() = 0;
};

}  // namespace core

