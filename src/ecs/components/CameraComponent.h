#pragma once

// Author: Karl-Johan Bailey
//
// CameraComponent (descriptive only)
// Stores camera settings. A camera orchestrator (outside this component) decides which camera is active.

#include "ecs/EntityId.h"
#include "math/Vec3.h"

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

  // Orthographic:
  float orthographicSize = 8.0f;

  // Shared:
  float nearClipPlane = 0.1f;
  float farClipPlane = 1000.0f;
  float aspectRatio = 16.0f / 9.0f;
  bool useFramebufferAspectRatio = true;

  // Engine-defined layer mask for what this camera can see.
  std::uint32_t visibleLayers = 0xFFFFFFFFu;

  bool frustumCulling = true;
  bool occlusionCulling = false;

  float shadowDistance = 50.0f;
  float lodBias = 1.0f;

  float renderScale = 1.0f;

  struct DepthOfFieldSettings final {
    enum class FocusMode {
      ManualDistance,
      TargetEntity,
    };

    bool enabled = false;
    FocusMode focusMode = FocusMode::ManualDistance;
    float focusDistance = 6.0f;
    float focusRange = 2.5f;
    float blurStrength = 0.65f;
    EntityId focusTarget = kInvalidEntityId;
    math::Vec3 focusTargetOffset{0.0f, 0.0f, 0.0f};
  };

  struct MotionBlurSettings final {
    bool enabled = false;
    float strength = 0.75f;
    float maxBlurPixels = 18.0f;
    int samples = 12;
  };

  DepthOfFieldSettings depthOfField{};
  MotionBlurSettings motionBlur{};
};

}  // namespace ecs
