#pragma once

// Author: Karl-Johan Bailey
//
// ShaderComponent (descriptive only)
// Describes how a mesh should be rendered. A renderer interprets these values.
//
// This is intended to support high-quality graphics by allowing:
// - PBR-style textures (albedo/normal/orm/height/emissive)
// - Parameter blocks (colors, roughness/metallic, tiling, etc)
// - Render state controls (depth, blend, cull, shadows)

#include "render/AssetRef.h"
#include "render/MaterialParameter.h"
#include "render/RenderState.h"
#include "render/TextureBinding.h"

#include <vector>

namespace ecs {

struct ShaderComponent {
  struct LodBreakpoint final {
    // Activate when the camera is at or beyond this distance from the renderable.
    // Breakpoints should be authored in ascending order so later entries win.
    float distanceMeters = 0.0f;

    // Optional overrides that are merged on top of the base material.
    std::vector<render::TextureBinding> textures;
    std::vector<render::MaterialParameter> parameters;

    // Optional tessellation override for shaders that support it.
    bool overrideTessellation = false;
    float tessNear = 0.0f;
    float tessFar = 0.0f;
    float tessMin = 0.0f;
    float tessMax = 0.0f;
    int tessQuality = 0;
  };

  bool enabled = true;

  // Shader asset reference (engine-defined).
  render::AssetRef shader;

  // Common render pipeline state.
  render::RenderMode renderMode = render::RenderMode::Opaque;
  render::CullMode cullMode = render::CullMode::Back;
  render::DepthTest depthTest = render::DepthTest::LessEqual;
  bool depthWrite = true;
  render::BlendMode blendMode = render::BlendMode::Disabled;
  bool doubleSided = false;

  bool receiveShadows = true;
  bool castShadows = true;

  // Texture slots (engine-defined names).
  std::vector<render::TextureBinding> textures;

  // Generic parameters for high-quality materials/shaders.
  std::vector<render::MaterialParameter> parameters;

  // Ordered distance breakpoints for LOD-style material overrides.
  std::vector<LodBreakpoint> lodBreakpoints;
};

}  // namespace ecs
