#include "core/ConsoleInputService.h"
#include "core/GameLoopService.h"
#include "core/SteadyTimeSource.h"

// Author: Karl-Johan Bailey

#include <iostream>

namespace {

class DemoGame final : public core::IGame {
 public:
  void onStart() override { std::cout << "Duppy Conquerer loop started. Type 'q' + Enter to quit.\n"; }

  void onTick(const core::TickContext& ctx) override {
    // This is where your per-frame gameplay/simulation code goes.
    // - Use ctx.deltaSeconds for time-based movement/animation.
    // - Use ctx.elapsedSeconds for timers.
    // - Read inputs from ctx.inputLines (or swap in a different input service later).

    for (const auto& line : ctx.inputLines) {
      if (!line.empty() && line != "q" && line != "quit" && line != "exit") {
        std::cout << "\ncommand: " << line << "\n";
      }
    }

    if (ctx.elapsedSeconds >= 30.0) {
      std::cout << "\nAuto-quit after 30 seconds (demo).\n";
      ctx.requestQuit();
    }
  }

  void onStop() override { std::cout << "Stopped.\n"; }
};

}  // namespace

int main() {
  // Concrete service implementations (can be swapped later via the interfaces).
  core::SteadyTimeSource timeSource;
  core::ConsoleInputService inputService;
  DemoGame game;

  // Loop settings (target FPS + optional frame cap).
  core::GameLoopConfig loopConfig;
  loopConfig.targetFps = 60.0;
  loopConfig.capFrameRate = true;

  // Debug HUD settings (what the loop prints each frame).
  core::DebugConfig debug;
  debug.enabled = true;
  debug.showFps = true;
  debug.showDeltaSeconds = true;
  debug.showFrameIndex = false;
  debug.showLastInput = true;
  debug.printEveryNFrames = 1;

  core::GameLoopService loop(timeSource, inputService, game, loopConfig, debug);
  loop.run();
  return 0;
}
