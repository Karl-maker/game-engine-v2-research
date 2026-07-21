#pragma once

namespace core {

  struct TickContext;

  class IGame {
  public:
    virtual ~IGame() = default;
    virtual void onStart() {}
    virtual void onTick(const TickContext& ctx) = 0;
    virtual void onStop() {}
  };

}  // namespace core

