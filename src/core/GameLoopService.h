#pragma once

// Author: Karl-Johan Bailey
//
// Game Loop Service
// - Owns the main "tick" loop and timing.
// - Pulls input from an injected input service.
// - Calls an injected game instance each frame (dependency inversion).
// - Optionally prints a tiny debug HUD (configurable).
//
// Where game logic goes:
// - Implement `core::IGame` and put gameplay/simulation updates in `onTick(...)`.
// - Read inputs from `TickContext::inputLines` (or swap in a different input service later).
// - Call `TickContext::requestQuit()` to exit the loop.

#include "DebugConfig.h"
#include "IGame.h"
#include "IInputService.h"
#include "ITimeSource.h"
#include "TickContext.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace core {

struct GameLoopConfig {
  double targetFps = 144.0;
  bool capFrameRate = true;
};

class GameLoopService final {
 public:
  GameLoopService(ITimeSource& timeSource,
                  IInputService& inputService,
                  IGame& game,
                  GameLoopConfig config,
                  DebugConfig debug);

  void run();
  void requestQuit();

 private:
  // Debug output is intentionally kept in the loop service so game code stays clean.
  void maybePrintDebug(double fps, double deltaSeconds, std::uint64_t frameIndex);

  ITimeSource& m_timeSource;
  IInputService& m_inputService;
  IGame& m_game;
  GameLoopConfig m_config;
  DebugConfig m_debug;

  std::atomic<bool> m_quitRequested{false};
  std::string m_lastInput;
  double m_lastDebugPrintSeconds = 0.0;
};

}  // namespace core
