#include "core/ControlService.h"

// Author: Karl-Johan Bailey

#include "core/TickContext.h"

#include <cctype>
#include <cstdlib>
#include <string_view>

namespace core {

namespace {

static std::string_view trim(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) s.remove_prefix(1);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0) s.remove_suffix(1);
  return s;
}

static bool parseFloat(std::string_view s, float& out) {
  s = trim(s);
  if (s.empty()) return false;
  std::string tmp(s);
  char* end = nullptr;
  const float value = std::strtof(tmp.c_str(), &end);
  if (!end) return false;
  if (*end != '\0') return false;
  out = value;
  return true;
}

static bool splitFirst(std::string_view s, std::string_view& head, std::string_view& tail) {
  s = trim(s);
  if (s.empty()) return false;
  const auto sp = s.find_first_of(" \t");
  if (sp == std::string_view::npos) {
    head = s;
    tail = {};
    return true;
  }
  head = s.substr(0, sp);
  tail = trim(s.substr(sp + 1));
  return true;
}

}  // namespace

bool ControlService::parseVec3(const std::string& s, math::Vec3& out) {
  std::string_view tail = s;
  std::string_view aS, bS, cS;
  if (!splitFirst(tail, aS, tail)) return false;
  if (!splitFirst(tail, bS, tail)) return false;
  if (!splitFirst(tail, cS, tail)) return false;
  float a = 0, b = 0, c = 0;
  if (!parseFloat(aS, a) || !parseFloat(bS, b) || !parseFloat(cS, c)) return false;
  out = {a, b, c};
  return true;
}

bool ControlService::parseTwoFloats(const std::string& s, float& a, float& b) {
  std::string_view tail = s;
  std::string_view aS, bS;
  if (!splitFirst(tail, aS, tail)) return false;
  if (!splitFirst(tail, bS, tail)) return false;
  return parseFloat(aS, a) && parseFloat(bS, b);
}

void ControlService::update(const TickContext& ctx, const RealtimeInput* realtime) {
  m_state.lookDeltaDeg = {0.0f, 0.0f, 0.0f};

  if (realtime && realtime->hasInput) {
    m_state.moveDirection = {realtime->moveX, 0.0f, realtime->moveZ};
    m_state.sprint = realtime->sprint;
    m_state.crouch = realtime->crouch;
    if (realtime->lookActive) {
      m_state.lookDeltaDeg.x += realtime->lookDeltaPitchDeg;
      m_state.lookDeltaDeg.y += realtime->lookDeltaYawDeg;
    }
  }

  for (const auto& raw : ctx.inputLines) {
    std::string_view line = trim(raw);
    if (line.empty()) continue;

    std::string_view cmd, args;
    (void)splitFirst(line, cmd, args);

    if (cmd == "stop") {
      m_state.moveDirection = {0.0f, 0.0f, 0.0f};
      m_state.sprint = false;
      m_state.crouch = false;
      continue;
    }

    if (cmd == "w") {
      m_state.moveDirection = {0.0f, 0.0f, 1.0f};
      continue;
    }
    if (cmd == "s") {
      m_state.moveDirection = {0.0f, 0.0f, -1.0f};
      continue;
    }
    if (cmd == "a") {
      m_state.moveDirection = {-1.0f, 0.0f, 0.0f};
      continue;
    }
    if (cmd == "d") {
      m_state.moveDirection = {1.0f, 0.0f, 0.0f};
      continue;
    }

    if (cmd == "move") {
      math::Vec3 v{};
      if (parseVec3(std::string(args), v)) {
        m_state.moveDirection = v;
      }
      continue;
    }

    if (cmd == "look") {
      float pitch = 0.0f;
      float yaw = 0.0f;
      if (parseTwoFloats(std::string(args), pitch, yaw)) {
        m_state.lookDeltaDeg.x += pitch;
        m_state.lookDeltaDeg.y += yaw;
      }
      continue;
    }

    if (cmd == "sprint") {
      m_state.sprint = true;
      m_state.crouch = false;
      continue;
    }
    if (cmd == "walk") {
      m_state.sprint = false;
      m_state.crouch = false;
      continue;
    }
    if (cmd == "crouch") {
      m_state.crouch = true;
      m_state.sprint = false;
      continue;
    }
  }
}

}  // namespace core
