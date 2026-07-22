#include "GameLoopService.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

namespace core {

GameLoopService::GameLoopService(ITimeSource& timeSource,
                                 IInputService& inputService,
                                 IGame& game,
                                 GameLoopConfig config,
                                 DebugConfig debug)
    : m_timeSource(timeSource),
      m_inputService(inputService),
      m_game(game),
      m_config(config),
      m_debug(debug) {}

void GameLoopService::requestQuit() { m_quitRequested.store(true, std::memory_order_relaxed); }

void GameLoopService::maybePrintDebug(double fps, double deltaSeconds, std::uint64_t frameIndex) {
  if (!m_debug.enabled) {
    return;
  }
  if (m_debug.printEveryNFrames > 1 && (frameIndex % static_cast<std::uint64_t>(m_debug.printEveryNFrames)) != 0) {
    return;
  }

  std::cout << "\r";
  bool first = true;

  auto emit = [&](const char* label, const std::string& value) {
    if (!first) std::cout << " | ";
    first = false;
    std::cout << label << value;
  };

  if (m_debug.showFps) {
    emit("fps=", std::to_string(static_cast<int>(fps + 0.5)));
  }
  if (m_debug.showDeltaSeconds) {
    emit("dt_ms=", std::to_string(static_cast<int>(deltaSeconds * 1000.0 + 0.5)));
  }
  if (m_debug.showFrameIndex) {
    emit("frame=", std::to_string(frameIndex));
  }
  if (m_debug.showLastInput) {
    emit("input=", m_lastInput.empty() ? "-" : m_lastInput);
  }

  std::cout << "    " << std::flush;
}

void GameLoopService::run() {
  // Boot services and game lifecycle.
  m_inputService.start();
  m_game.onStart();

  // Frame pacing settings.
  const double minFrameSeconds = (m_config.targetFps > 0.0) ? (1.0 / m_config.targetFps) : 0.0;
  const double startSeconds = m_timeSource.nowSeconds();
  double lastFrameStart = startSeconds;

  std::uint64_t frameIndex = 0;

  // Simple FPS estimator (rolling 1s-ish window).
  double fpsWindowStart = startSeconds;
  std::uint64_t fpsWindowFrames = 0;
  double fpsEstimate = 0.0;
  double lastWorkSeconds = 0.0;

  while (!m_quitRequested.load(std::memory_order_relaxed)) {
    // --- Timing (delta / elapsed) ---
    const double frameStart = m_timeSource.nowSeconds();
    double deltaSeconds = frameStart - lastFrameStart;
    lastFrameStart = frameStart;

    // Clamp huge deltas (e.g. breakpoint / OS stall) so simulation doesn't explode.
    deltaSeconds = std::clamp(deltaSeconds, 0.0, 0.25);

    // --- Input collection ---
    auto lines = m_inputService.drainLines();
    if (!lines.empty()) {
      m_lastInput = lines.back();
      for (const auto& line : lines) {
        // Default quit commands (demo-friendly).
        if (line == "q" || line == "quit" || line == "exit") {
          requestQuit();
        }
      }
    }

    // --- Tick context passed into the game ---
    TickContext ctx;
    ctx.deltaSeconds = deltaSeconds;
    ctx.elapsedSeconds = frameStart - startSeconds;
    ctx.frameIndex = frameIndex;
    ctx.inputLines = std::move(lines);
    ctx.debugHudEnabled = m_debug.enabled;
    ctx.fpsEstimate = fpsEstimate;
    ctx.cpuWorkSeconds = lastWorkSeconds;
    ctx.requestQuit = [this] { requestQuit(); };

    // --- GAME LOGIC ENTRY POINT ---
    // Put your gameplay/simulation update inside your IGame::onTick implementation.
    m_game.onTick(ctx);

    // --- FPS estimation ---
    fpsWindowFrames += 1;
    const double fpsNow = m_timeSource.nowSeconds();
    const double fpsWindowSeconds = fpsNow - fpsWindowStart;
    if (fpsWindowSeconds >= 1.0) {
      fpsEstimate = static_cast<double>(fpsWindowFrames) / fpsWindowSeconds;
      fpsWindowStart = fpsNow;
      fpsWindowFrames = 0;
    }

    const double workNow = m_timeSource.nowSeconds();
    lastWorkSeconds = workNow - frameStart;

    maybePrintDebug(fpsEstimate, deltaSeconds, frameIndex);
    frameIndex += 1;

    // --- Frame pacing (optional) ---
    // Keeps CPU usage down and makes the loop run near target FPS.
    if (m_config.capFrameRate && minFrameSeconds > 0.0) {
      const double frameEnd = m_timeSource.nowSeconds();
      const double workSeconds = frameEnd - frameStart;
      const double sleepSeconds = minFrameSeconds - workSeconds;
      if (sleepSeconds > 0.0) {
        std::this_thread::sleep_for(std::chrono::duration<double>(sleepSeconds));
      }
    }
  }

  std::cout << "\n";

  // Game shutdown lifecycle.
  m_game.onStop();
}

}  // namespace core
