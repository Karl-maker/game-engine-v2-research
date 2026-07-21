#include "core/ConsoleInputService.h"
#include "core/GameLoopService.h"
#include "core/SteadyTimeSource.h"
#include "ecs/EntityRegistry.h"

// Author: Karl-Johan Bailey

#include <iostream>
#include <string_view>

namespace {

class DemoGame final : public core::IGame {
 public:
  void onStart() override {
    std::cout << "Duppy Conquerer loop started. Type 'q' + Enter to quit.\n";

    m_player = m_registry.createEntity("Player");
    std::cout << "Created entity id=" << m_registry.identity(m_player).id << " name=" << m_registry.identity(m_player).name
              << "\n";
    std::cout << "Try: `name <newName>` then Enter.\n";
  }

  void onTick(const core::TickContext& ctx) override {
    // This is where your per-frame gameplay/simulation code goes.
    // - Use ctx.deltaSeconds for time-based movement/animation.
    // - Use ctx.elapsedSeconds for timers.
    // - Read inputs from ctx.inputLines (or swap in a different input service later).

    for (const auto& line : ctx.inputLines) {
      if (line.empty()) continue;
      if (line == "q" || line == "quit" || line == "exit") continue;

      if (startsWith(line, "name ")) {
        const auto newName = line.substr(5);
        m_registry.deferMutate<ecs::IdentityComponent>(m_player, [newName](ecs::IdentityComponent& idc) {
          idc.name = newName;
        });
        continue;
      }

      std::cout << "\ncommand: " << line << "\n";
    }

    // Apply queued entity/component edits at a safe point (end-of-frame sync).
    m_registry.applyDeferred();

    if (m_registry.isAlive(m_player)) {
      const auto& ident = m_registry.identity(m_player);
      if (ident.name != m_lastName) {
        m_lastName = ident.name;
        std::cout << "\nentity " << ident.id << " renamed to " << ident.name << "\n";
      }
    }

    if (ctx.elapsedSeconds >= 30.0) {
      std::cout << "\nAuto-quit after 30 seconds (demo).\n";
      ctx.requestQuit();
    }
  }

  void onStop() override { std::cout << "Stopped.\n"; }

 private:
  static bool startsWith(const std::string& s, std::string_view prefix) {
    return s.size() >= prefix.size() && std::string_view(s.data(), prefix.size()) == prefix;
  }

  ecs::EntityRegistry m_registry;
  ecs::EntityId m_player = ecs::kInvalidEntityId;
  std::string m_lastName;
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
