#pragma once

// Author: Karl-Johan Bailey
//
// Data passed into `IGame::onTick` each frame.
// Keep this small and focused; expand as you add systems (rendering, audio, etc).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace core {

struct TickContext {
  double deltaSeconds = 0.0;
  double elapsedSeconds = 0.0;
  std::uint64_t frameIndex = 0;
  std::vector<std::string> inputLines;

  std::function<void()> requestQuit;
};

}  // namespace core
