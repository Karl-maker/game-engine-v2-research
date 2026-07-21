// Author: Karl-Johan Bailey

#include "core/ConsoleInputService.h"
#include "core/GameLoopService.h"
#include "core/SteadyTimeSource.h"
#include "games/AttachmentDemoGame.h"
#include "games/EnemyFollowDemoGame.h"
#include "games/TargetDemoGame.h"

#include <iostream>
#include <memory>
#include <string>

static std::unique_ptr<core::IGame> makeGameFromArgs(int argc, char** argv) {
  // Usage:
  //   ./duppy attachment
  //   ./duppy target
  //
  // Default is "attachment".
  const std::string mode = (argc >= 2) ? std::string(argv[1]) : "attachment";
  if (mode == "enemy" || mode == "enemy-follow" || mode == "3") {
    return std::make_unique<games::EnemyFollowDemoGame>();
  }
  if (mode == "target" || mode == "2") {
    return std::make_unique<games::TargetDemoGame>();
  }
  return std::make_unique<games::AttachmentDemoGame>();
}

int main(int argc, char** argv) {
  std::cout << "Duppy Conquerer (base loop)\n";
  std::cout << "Run a demo:\n";
  std::cout << "  ./duppy attachment\n";
  std::cout << "  ./duppy target\n";
  std::cout << "  ./duppy enemy\n";
  std::cout << "Type `q` then Enter to quit.\n\n";

  core::SteadyTimeSource timeSource;
  core::ConsoleInputService inputService;
  auto game = makeGameFromArgs(argc, argv);

  core::GameLoopConfig loopConfig;
  loopConfig.targetFps = 60.0;
  loopConfig.capFrameRate = true;

  const std::string mode = (argc >= 2) ? std::string(argv[1]) : "attachment";

  core::DebugConfig debug;
  // The enemy demo renders its own full-screen table, so disable the loop HUD there.
  debug.enabled = !(mode == "enemy" || mode == "enemy-follow" || mode == "3");
  debug.showFps = true;
  debug.showDeltaSeconds = true;
  debug.showFrameIndex = false;
  debug.showLastInput = true;
  debug.printEveryNFrames = 1;

  core::GameLoopService loop(timeSource, inputService, *game, loopConfig, debug);
  loop.run();
  return 0;
}
