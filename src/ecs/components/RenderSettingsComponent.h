#pragma once

// Author: Karl-Johan Bailey
//
// RenderSettingsComponent (demo)
// Global-ish render switches for the demo renderer.

namespace ecs {

struct RenderSettingsComponent final {
  bool enabled = true;

  // Shadows
  // Default is disabled to preserve the current look (no shadow maps yet in the demo scenes).
  bool shadowsEnabled = true;
  int shadowQuality = 0;  // 0=Low,1=Medium,2=High (renderer-defined)
  float shadowStrength = 0.3f;

  // If true, the shadow caster pass may use tessellation for terrain when available.
  bool shadowUseTessellation = false;
};

}  // namespace ecs

