// Author: Karl-Johan Bailey

#include "core/ConsoleInputService.h"
#include "core/GameLoopService.h"
#include "core/SteadyTimeSource.h"
#include "games/AttachmentDemoGame.h"
#include "games/EnemyFollowDemoGame.h"
#include "games/GameplayDemoGame.h"
#include "games/TargetDemoGame.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

namespace {

struct AppConfig final {
  core::GameLoopConfig loop{};
  games::GameplayDemoConfig gameplay{};
};

static bool isModeToken(const std::string& s) {
  return s == "attachment" || s == "target" || s == "enemy" || s == "enemy-follow" || s == "gameplay" || s == "graphics" ||
         s == "2" || s == "3" || s == "4";
}

static std::unique_ptr<core::IGame> makeGameFromMode(const std::string& mode, const AppConfig& cfg) {
  if (mode == "enemy" || mode == "enemy-follow" || mode == "3") {
    return std::make_unique<games::EnemyFollowDemoGame>();
  }
  if (mode == "gameplay" || mode == "graphics" || mode == "4") {
    return std::make_unique<games::GameplayDemoGame>(cfg.gameplay);
  }
  if (mode == "target" || mode == "2") {
    return std::make_unique<games::TargetDemoGame>();
  }
  return std::make_unique<games::AttachmentDemoGame>();
}

}  // namespace

int main(int argc, char** argv) {
  std::cout << "Duppy Conquerer (base loop)\n";
  std::cout << "Run a demo:\n";
  std::cout << "  ./duppy attachment\n";
  std::cout << "  ./duppy target\n";
  std::cout << "  ./duppy enemy\n";
  std::cout << "  ./duppy gameplay\n";
  std::cout << "Flags:\n";
  std::cout << "  --fullscreen | --windowed\n";
  std::cout << "  --vsync | --no-vsync\n";
  std::cout << "  --uncapped | --cap-fps\n";
  std::cout << "  --fps N\n";
  std::cout << "  --width N  --height N\n";
  std::cout << "  --chunks PATH\n";
  std::cout << "  --chunk-size N\n";
  std::cout << "  --chunk-search-radius N\n";
  std::cout << "  --chunk-load-proximity N\n";
  std::cout << "  --chunk-unload-proximity N\n";
  std::cout << "Type `q` then Enter to quit.\n\n";

  core::SteadyTimeSource timeSource;
  core::ConsoleInputService inputService;

  AppConfig cfg;
  cfg.loop.targetFps = 144.0;
  cfg.loop.capFrameRate = false;  // default: do not artificially cap
  cfg.gameplay.fullscreen = false;
  cfg.gameplay.vsync = false;     // default: avoid 60fps lock from vsync
  cfg.gameplay.windowWidth = 1280;
  cfg.gameplay.windowHeight = 720;
  cfg.gameplay.fullscreenRefreshRateHz = 0;

  std::string mode = "attachment";
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i] ? std::string(argv[i]) : std::string{};
    if (arg.empty()) continue;

    if (arg == "--fullscreen") {
      cfg.gameplay.fullscreen = true;
      continue;
    }
    if (arg == "--windowed") {
      cfg.gameplay.fullscreen = false;
      continue;
    }
    if (arg == "--vsync") {
      cfg.gameplay.vsync = true;
      continue;
    }
    if (arg == "--no-vsync") {
      cfg.gameplay.vsync = false;
      continue;
    }
    if (arg == "--uncapped") {
      cfg.loop.capFrameRate = false;
      continue;
    }
    if (arg == "--cap-fps") {
      cfg.loop.capFrameRate = true;
      continue;
    }
    if (arg == "--fps" && i + 1 < argc) {
      try {
        cfg.loop.targetFps = std::stod(argv[++i]);
      } catch (...) {
      }
      continue;
    }
    if (arg == "--width" && i + 1 < argc) {
      try {
        cfg.gameplay.windowWidth = std::max(1, std::stoi(argv[++i]));
      } catch (...) {
      }
      continue;
    }
    if (arg == "--height" && i + 1 < argc) {
      try {
        cfg.gameplay.windowHeight = std::max(1, std::stoi(argv[++i]));
      } catch (...) {
      }
      continue;
    }
    if (arg == "--chunks" && i + 1 < argc) {
      cfg.gameplay.chunkConfigPath = std::string(argv[++i] ? argv[i] : "");
      continue;
    }
    if (arg == "--chunk-size" && i + 1 < argc) {
      try {
        cfg.gameplay.chunkSizeMeters = static_cast<float>(std::stod(argv[++i]));
      } catch (...) {
      }
      continue;
    }
    if (arg == "--chunk-search-radius" && i + 1 < argc) {
      try {
        cfg.gameplay.chunkSearchRadius = std::max(0, std::stoi(argv[++i]));
      } catch (...) {
      }
      continue;
    }
    if (arg == "--chunk-load-proximity" && i + 1 < argc) {
      try {
        cfg.gameplay.chunkLoadProximityMeters = static_cast<float>(std::stod(argv[++i]));
      } catch (...) {
      }
      continue;
    }
    if (arg == "--chunk-unload-proximity" && i + 1 < argc) {
      try {
        cfg.gameplay.chunkUnloadProximityMeters = static_cast<float>(std::stod(argv[++i]));
      } catch (...) {
      }
      continue;
    }

    if (arg.rfind("--", 0) == 0) {
      continue;  // unknown flag: ignore (demo-friendly)
    }
    if (isModeToken(arg)) {
      mode = arg;
      continue;
    }
  }

  auto game = makeGameFromMode(mode, cfg);

  core::DebugConfig debug;
  // The enemy demo renders its own full-screen table, so disable the loop HUD there.
  debug.enabled = !(mode == "enemy" || mode == "enemy-follow" || mode == "3");
  debug.showFps = true;
  debug.showDeltaSeconds = true;
  debug.showFrameIndex = false;
  debug.showLastInput = true;
  debug.printEveryNFrames = 1;
  debug.minSecondsBetweenPrints = 0.10;
  debug.enabled = true;

  core::GameLoopService loop(timeSource, inputService, *game, cfg.loop, debug);
  loop.run();
  return 0;
}
