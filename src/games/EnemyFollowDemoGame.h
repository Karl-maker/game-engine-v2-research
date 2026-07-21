#pragma once

// Author: Karl-Johan Bailey
//
// EnemyFollowDemoGame (single-file demo)
// - Player can walk/run using terminal commands.
// - Enemies follow the player and damage health when close.
// - Renders a terminal table each update, clearing the screen and coloring rows red as enemies get closer.
//
// Controls (type then press Enter):
// - `w`, `a`, `s`, `d` : move
// - `run` / `walk`    : toggle move mode
// - `spawn <n>`       : spawn more enemies
// - `help`            : show help
// - `q`               : quit (built-in to loop)

#include "core/IGame.h"
#include "core/TickContext.h"

#include "ecs/EntityRegistry.h"
#include "ecs/components/ControllerComponent.h"
#include "ecs/components/IdentityComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/StatsComponent.h"
#include "ecs/components/TransformComponent.h"

#include "math/Vec3.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace games {

class EnemyFollowDemoGame final : public core::IGame {
 public:
  void onStart() override {
    printHelp();

    // Player setup
    m_player = m_registry.createEntity("player");
    auto& pTr = m_registry.emplace<ecs::TransformComponent>(m_player);
    pTr.position = {0.0f, 0.0f, 0.0f};

    auto& stats = m_registry.emplace<ecs::StatsComponent>(m_player);
    stats.maxHealth = 100.0f;
    stats.health = 100.0f;
    stats.walkingSpeed = 2.2f;
    stats.runningSpeed = 4.2f;

    m_registry.emplace<ecs::MotionComponent>(m_player);

    auto& ctrl = m_registry.emplace<ecs::ControllerComponent>(m_player);
    ctrl.mode = ecs::ControllerComponent::Mode::Player;
    ctrl.enabled = true;
    ctrl.priority = 100;

    // Enemies
    spawnEnemies(4);

    // Initial render.
    renderGrid();
  }

  void onTick(const core::TickContext& ctx) override {
    // Service: translate raw terminal input lines into ControllerComponent requests.
    controllerService_updateFromInput(ctx.inputLines);

    // Systems: apply controller -> motion, AI follow -> motion, then motion -> transform.
    system_applyControllerToMotion();
    system_aiFollowPlayer();
    system_integrateMotion(ctx.deltaSeconds);
    system_damagePlayer(ctx.deltaSeconds);

    // Render at a reasonable frequency so the terminal stays readable.
    m_renderAccumulator += ctx.deltaSeconds;
    if (m_renderAccumulator >= m_renderEverySeconds) {
      m_renderAccumulator = 0.0;
      renderGrid();
    }

    // End condition.
    auto* stats = m_registry.tryGet<ecs::StatsComponent>(m_player);
    if (stats && stats->health <= 0.0f) {
      clearScreen();
      std::cout << "You died.\n";
      std::cout << "Type `q` then Enter to quit.\n";
      ctx.requestQuit();
    }
  }

  void onStop() override { std::cout << "\nStopped enemy-follow demo.\n"; }

 private:
  // -----------------------------
  // Terminal helpers
  // -----------------------------
  static void clearScreen() {
    // ANSI clear screen + cursor home.
    std::cout << "\033[2J\033[H";
  }

  static std::string trim(std::string s) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
  }

  static bool startsWith(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
  }

  static float clamp(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }

  static float length2D(const math::Vec3& v) { return std::sqrt(v.x * v.x + v.z * v.z); }

  static math::Vec3 normalize2D(const math::Vec3& v) {
    const float len = length2D(v);
    if (len <= 0.00001f) return {0.0f, 0.0f, 0.0f};
    return {v.x / len, 0.0f, v.z / len};
  }

  static int toIntOr(const std::string& s, int fallback) {
    try {
      return std::stoi(s);
    } catch (...) {
      return fallback;
    }
  }

  static const char* colorForDistance(float d) {
    // Closer => hotter.
    if (d < 1.2f) return "\033[31m";   // red
    if (d < 2.5f) return "\033[33m";   // yellow
    return "\033[37m";                // white
  }

  static const char* colorForPlayer() { return "\033[36m"; }  // cyan

  static constexpr const char* kColorReset = "\033[0m";

  void printHelp() const {
    clearScreen();
    std::cout << "Enemy-follow demo\n";
    std::cout << "Commands:\n";
    std::cout << "  w/a/s/d        move one step of intent\n";
    std::cout << "  run | walk     movement mode\n";
    std::cout << "  spawn <n>      spawn enemies\n";
    std::cout << "  help           show this\n";
    std::cout << "  q              quit\n\n";
  }

  // -----------------------------
  // Setup helpers
  // -----------------------------
  void spawnEnemies(int count) {
    count = std::max(0, count);
    for (int i = 0; i < count; ++i) {
      const int idx = static_cast<int>(m_enemies.size()) + 1;
      const auto name = std::string("enemy_") + std::to_string(idx);

      const ecs::EntityId e = m_registry.createEntity(name);
      auto& tr = m_registry.emplace<ecs::TransformComponent>(e);

      // Spread enemies around player.
      const float angle = static_cast<float>(idx) * 1.7f;
      const float radius = 6.0f + static_cast<float>(idx % 3) * 2.5f;
      tr.position = {std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};

      auto& m = m_registry.emplace<ecs::MotionComponent>(e);
      m.mode = ecs::MotionComponent::Mode::Running;

      auto& s = m_registry.emplace<ecs::StatsComponent>(e);
      s.walkingSpeed = 1.8f;
      s.runningSpeed = 3.1f;

      m_enemies.push_back(e);
    }
  }

  // -----------------------------
  // Controller "service"
  // -----------------------------
  void controllerService_updateFromInput(const std::vector<std::string>& lines) {
    auto* ctrl = m_registry.tryGet<ecs::ControllerComponent>(m_player);
    auto* motion = m_registry.tryGet<ecs::MotionComponent>(m_player);
    if (!ctrl || !motion || !ctrl->enabled) return;

    // Reset transient requests each frame; control service fills them.
    ctrl->moveRequest.hasRequest = false;
    ctrl->moveRequest.hasDirection = false;

    for (const auto& raw : lines) {
      const std::string line = trim(raw);
      if (line.empty()) continue;

      if (line == "help") {
        printHelp();
        continue;
      }
      if (line == "run") {
        ctrl->moveRequest.moveMode = ecs::ControllerComponent::MoveMode::Sprint;
        motion->mode = ecs::MotionComponent::Mode::Running;
        continue;
      }
      if (line == "walk") {
        ctrl->moveRequest.moveMode = ecs::ControllerComponent::MoveMode::Walk;
        motion->mode = ecs::MotionComponent::Mode::Walking;
        continue;
      }
      if (startsWith(line, "spawn ")) {
        const int n = toIntOr(std::string(line.substr(6)), 1);
        spawnEnemies(n);
        continue;
      }

      // WASD: set a direction request for this frame.
      math::Vec3 dir{0.0f, 0.0f, 0.0f};
      if (line == "w") dir.z += 1.0f;
      if (line == "s") dir.z -= 1.0f;
      if (line == "d") dir.x += 1.0f;
      if (line == "a") dir.x -= 1.0f;

      if (dir.x != 0.0f || dir.z != 0.0f) {
        ctrl->moveRequest.hasRequest = true;
        ctrl->moveRequest.hasDirection = true;
        ctrl->moveRequest.direction = normalize2D(dir);
      }
    }
  }

  // -----------------------------
  // Systems (demo)
  // -----------------------------
  void system_applyControllerToMotion() {
    // Controller -> Motion is where input intent becomes movement state.
    m_registry.view<ecs::ControllerComponent, ecs::MotionComponent>(
        [&](ecs::EntityId id, ecs::ControllerComponent& ctrl, ecs::MotionComponent& motion) {
          if (id != m_player) return;
          if (!ctrl.enabled) return;

          if (ctrl.moveRequest.hasRequest && ctrl.moveRequest.hasDirection) {
            motion.desiredDirection = ctrl.moveRequest.direction;
            motion.isMoving = true;
            motion.movementState = ecs::MotionComponent::MovementState::Moving;
          } else {
            motion.desiredDirection = {0.0f, 0.0f, 0.0f};
            motion.isMoving = false;
            motion.movementState = ecs::MotionComponent::MovementState::Idle;
          }
        });
  }

  void system_aiFollowPlayer() {
    const auto* playerTr = m_registry.tryGet<ecs::TransformComponent>(m_player);
    if (!playerTr) return;

    // Enemy follow: desiredDirection points toward the player.
    for (const auto enemy : m_enemies) {
      if (!m_registry.isAlive(enemy)) continue;
      auto* tr = m_registry.tryGet<ecs::TransformComponent>(enemy);
      auto* motion = m_registry.tryGet<ecs::MotionComponent>(enemy);
      if (!tr || !motion) continue;

      const math::Vec3 toPlayer{playerTr->position.x - tr->position.x, 0.0f, playerTr->position.z - tr->position.z};
      const float d = length2D(toPlayer);
      if (d > 0.8f) {
        motion->desiredDirection = normalize2D(toPlayer);
        motion->isMoving = true;
        motion->movementState = ecs::MotionComponent::MovementState::Moving;
      } else {
        motion->desiredDirection = {0.0f, 0.0f, 0.0f};
        motion->isMoving = false;
        motion->movementState = ecs::MotionComponent::MovementState::Idle;
      }
    }
  }

  void system_integrateMotion(double deltaSeconds) {
    const float dt = static_cast<float>(deltaSeconds);

    // Motion integration: apply desired direction to transform using stats speeds.
    m_registry.view<ecs::TransformComponent, ecs::MotionComponent, ecs::StatsComponent>(
        [&](ecs::EntityId id, ecs::TransformComponent& tr, ecs::MotionComponent& motion, ecs::StatsComponent& stats) {
          (void)id;
          float speed = 0.0f;
          if (motion.mode == ecs::MotionComponent::Mode::Running) speed = stats.runningSpeed;
          else if (motion.mode == ecs::MotionComponent::Mode::Walking) speed = stats.walkingSpeed;
          else if (motion.mode == ecs::MotionComponent::Mode::Swimming) speed = stats.swimmingSpeed;
          else speed = stats.walkingSpeed;

          const math::Vec3 dir = normalize2D(motion.desiredDirection);
          const math::Vec3 vel = {dir.x * speed, 0.0f, dir.z * speed};
          motion.velocity = vel;
          motion.currentSpeed = speed * length2D(dir);

          tr.position.x += motion.velocity.x * dt;
          tr.position.z += motion.velocity.z * dt;
        });
  }

  void system_damagePlayer(double deltaSeconds) {
    auto* playerStats = m_registry.tryGet<ecs::StatsComponent>(m_player);
    const auto* playerTr = m_registry.tryGet<ecs::TransformComponent>(m_player);
    if (!playerStats || !playerTr) return;

    float minDist = 9999.0f;
    for (const auto enemy : m_enemies) {
      const auto* tr = m_registry.tryGet<ecs::TransformComponent>(enemy);
      if (!tr) continue;
      const math::Vec3 d{tr->position.x - playerTr->position.x, 0.0f, tr->position.z - playerTr->position.z};
      minDist = std::min(minDist, length2D(d));
    }

    // Damage ramps up as enemies get close.
    if (minDist < 1.5f) {
      const float t = clamp((1.5f - minDist) / 1.5f, 0.0f, 1.0f);
      const float dps = 5.0f + 25.0f * t;
      playerStats->health = std::max(0.0f, playerStats->health - dps * static_cast<float>(deltaSeconds));
    }
  }

  // -----------------------------
  // Render
  // -----------------------------
  struct GridCell {
    int enemyCount = 0;
    float minEnemyDist = 9999.0f;
    bool hasPlayer = false;
  };

  void renderGrid() {
    const auto* playerStats = m_registry.tryGet<ecs::StatsComponent>(m_player);
    const auto* playerTr = m_registry.tryGet<ecs::TransformComponent>(m_player);
    const auto* playerMotion = m_registry.tryGet<ecs::MotionComponent>(m_player);
    if (!playerStats || !playerTr) return;

    clearScreen();

    std::cout << "Enemy-follow demo | health: " << std::fixed << std::setprecision(1) << playerStats->health << "/"
              << playerStats->maxHealth << "\n";
    if (playerMotion) {
      std::cout << "mode: "
                << (playerMotion->mode == ecs::MotionComponent::Mode::Running ? "running" : "walking")
                << " | Commands: w/a/s/d, run, walk, spawn <n>, help, q\n\n";
    } else {
      std::cout << "Commands: w/a/s/d, run, walk, spawn <n>, help, q\n\n";
    }

    // Grid settings
    constexpr int gridRadiusCells = 12;  // total size: (2R+1)
    constexpr int gridSize = gridRadiusCells * 2 + 1;
    constexpr float cellMeters = 1.0f;

    GridCell grid[gridSize][gridSize];

    auto worldToCell = [&](const math::Vec3& world) {
      const float relX = (world.x - playerTr->position.x) / cellMeters;
      const float relZ = (world.z - playerTr->position.z) / cellMeters;
      const int cx = static_cast<int>(std::lround(relX)) + gridRadiusCells;
      const int cz = static_cast<int>(std::lround(relZ)) + gridRadiusCells;
      return std::pair<int, int>{cx, cz};
    };

    // Mark player at the center.
    grid[gridRadiusCells][gridRadiusCells].hasPlayer = true;

    // Fill enemy cells (relative to player).
    for (const auto enemy : m_enemies) {
      if (!m_registry.isAlive(enemy)) continue;
      const auto* tr = m_registry.tryGet<ecs::TransformComponent>(enemy);
      if (!tr) continue;

      const auto [cx, cz] = worldToCell(tr->position);
      if (cx < 0 || cx >= gridSize || cz < 0 || cz >= gridSize) continue;

      const math::Vec3 d{tr->position.x - playerTr->position.x, 0.0f, tr->position.z - playerTr->position.z};
      const float dist = length2D(d);
      auto& cell = grid[cz][cx];
      cell.enemyCount += 1;
      cell.minEnemyDist = std::min(cell.minEnemyDist, dist);
    }

    // Render: top border
    std::cout << "Legend: " << colorForPlayer() << "P" << kColorReset << "=player, "
              << "\033[37m" << "e" << kColorReset << "=enemy, "
              << "\033[33m" << "yellow" << kColorReset << "=near, "
              << "\033[31m" << "red" << kColorReset << "=danger\n\n";

    std::cout << "+";
    for (int x = 0; x < gridSize; ++x) std::cout << "-";
    std::cout << "+\n";

    for (int z = 0; z < gridSize; ++z) {
      std::cout << "|";
      for (int x = 0; x < gridSize; ++x) {
        const auto& cell = grid[z][x];
        if (cell.hasPlayer) {
          std::cout << colorForPlayer() << "P" << kColorReset;
          continue;
        }
        if (cell.enemyCount <= 0) {
          std::cout << ".";
          continue;
        }

        const char ch = (cell.enemyCount >= 2 && cell.enemyCount <= 9) ? static_cast<char>('0' + cell.enemyCount) : 'e';
        std::cout << colorForDistance(cell.minEnemyDist) << ch << kColorReset;
      }
      std::cout << "|\n";
    }

    std::cout << "+";
    for (int x = 0; x < gridSize; ++x) std::cout << "-";
    std::cout << "+\n\n";

    std::cout << "player world: x=" << std::setprecision(2) << playerTr->position.x
              << " z=" << playerTr->position.z << " | enemies: " << m_enemies.size() << "\n";

    std::cout << std::flush;
  }

  ecs::EntityRegistry m_registry;
  ecs::EntityId m_player = ecs::kInvalidEntityId;
  std::vector<ecs::EntityId> m_enemies;

  double m_renderAccumulator = 0.0;
  const double m_renderEverySeconds = 0.10;
};

}  // namespace games
