#pragma once

// Author: Karl-Johan Bailey
//
// Console input implementation:
// - Spins a background thread that blocks on `std::getline(std::cin, ...)`.
// - Main loop stays non-blocking and "drains" any lines typed since last frame.
// - This is a simple starting point; swap it for SDL, platform events, etc. later
//   by implementing `core::IInputService`.

#include "IInputService.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>

namespace core {

class ConsoleInputService final : public IInputService {
 public:
  ConsoleInputService() = default;
  ~ConsoleInputService() override;

  void start() override;
  std::vector<std::string> drainLines() override;

 private:
  void readerThreadMain();

  std::atomic<bool> m_started{false};
  std::atomic<bool> m_stopping{false};
  std::thread m_readerThread;

  std::mutex m_mutex;
  std::deque<std::string> m_lines;
};

}  // namespace core
