#pragma once

namespace core {

struct DebugConfig {
  bool enabled = true;
  bool showFps = true;
  bool showDeltaSeconds = true;
  bool showFrameIndex = false;
  bool showLastInput = true;
  int printEveryNFrames = 1;
  double minSecondsBetweenPrints = 0.1;
};

}  // namespace core
