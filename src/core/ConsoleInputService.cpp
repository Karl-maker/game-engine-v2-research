#include "ConsoleInputService.h"

#include <iostream>
#include <utility>

namespace core {

ConsoleInputService::~ConsoleInputService() {
  m_stopping.store(true, std::memory_order_relaxed);

  if (m_readerThread.joinable()) {
    m_readerThread.detach();
  }
}

void ConsoleInputService::start() {
  bool expected = false;
  if (!m_started.compare_exchange_strong(expected, true)) {
    return;
  }

  m_readerThread = std::thread([this] { readerThreadMain(); });
}

std::vector<std::string> ConsoleInputService::drainLines() {
  std::vector<std::string> out;
  std::lock_guard<std::mutex> lock(m_mutex);
  while (!m_lines.empty()) {
    out.emplace_back(std::move(m_lines.front()));
    m_lines.pop_front();
  }
  return out;
}

void ConsoleInputService::readerThreadMain() {
  std::string line;
  while (!m_stopping.load(std::memory_order_relaxed) && std::getline(std::cin, line)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lines.emplace_back(line);
  }
}

}  // namespace core

