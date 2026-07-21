#pragma once

// Author: Karl-Johan Bailey
//
// CameraComponent (descriptive only)
// Stores camera settings. A camera orchestrator (outside this component) decides which camera is active.

#include <cstdint>

namespace ecs {

struct CameraComponent {
  enum class ProjectionType {
    Perspective,
    Orthographic,
  };

  ProjectionType projectionType = ProjectionType::Perspective;

  // Perspective:
  float fieldOfViewDeg = 60.0f;

  // Shared:
  float nearClipPlane = 0.1f;
  float farClipPlane = 1000.0f;
  float aspectRatio = 16.0f / 9.0f;

  // Engine-defined layer mask for what this camera can see.
  std::uint32_t visibleLayers = 0xFFFFFFFFu;

  bool frustumCulling = true;
  bool occlusionCulling = false;

  float shadowDistance = 50.0f;
  float lodBias = 1.0f;
};

}  // namespace ecs

