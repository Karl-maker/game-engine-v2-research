#pragma once

// Author: Karl-Johan Bailey
//
// RenderSettingsComponent (demo)
// Global-ish render switches for the demo renderer.

namespace ecs {

struct RenderSettingsComponent final {
  bool enabled = true;

  // Main render feature families.
  bool terrainEnabled = true;
  bool meshEnabled = true;
  bool rockEnabled = true;
  bool grassEnabled = true;
  bool billboardEnabled = true;
  bool farLodBillboardsEnabled = true;
  bool vfxEnabled = true;
  bool hudEnabled = true;
  bool postProcessingEnabled = true;

  // Shadows
  // Default is disabled to preserve the current look (no shadow maps yet in the demo scenes).
  bool shadowsEnabled = true;
  int shadowQuality = 2;  // 0=Low,1=Medium,2=High (renderer-defined)
  float shadowStrength = 0.8f;

  // If true, the shadow caster pass may use tessellation for terrain when available.
  bool shadowUseTessellation = false;

  // Debug-only visual overlays.
  bool showRays = false;
  bool showCollisionBoxes = false;
  bool showCombatBoxes = false;
  bool showSkeletonBones = false;
};

}  // namespace ecs
