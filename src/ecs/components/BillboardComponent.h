#pragma once

// Author: Karl-Johan Bailey
//
// BillboardComponent (descriptive only)
// Describes a textured world-space quad that can face the active camera.

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "render/AnimatedTexture.h"
#include "render/AssetRef.h"
#include "render/Color.h"

namespace ecs {

struct BillboardComponent final {
  enum class FaceMode {
    None,
    CameraPlane,
    YawOnly,
  };

  bool enabled = true;
  bool visible = true;
  bool textureEnabled = true;
  bool depthWrite = false;
  bool doubleSided = true;

  FaceMode faceMode = FaceMode::CameraPlane;
  math::Vec2 sizeMeters{1.25f, 1.25f};
  math::Vec2 pivot{0.5f, 0.0f};
  math::Vec3 worldOffset{0.0f, 0.0f, 0.0f};
  math::Vec3 rotationOffsetDeg{0.0f, 0.0f, 0.0f};
  float maxRenderDistance = 96.0f;

  render::AssetRef texture{};
  render::AnimatedTexture animatedTexture{};
  render::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
};

}  // namespace ecs
