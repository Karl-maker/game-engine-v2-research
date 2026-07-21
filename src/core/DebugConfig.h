#pragma once

namespace core {

struct DebugConfig {
  bool enabled = true;
  bool showFps = true;
  bool showDeltaSeconds = true;
  bool showFrameIndex = false;
  bool showLastInput = true;
  int printEveryNFrames = 1;
};

}  // namespace core

