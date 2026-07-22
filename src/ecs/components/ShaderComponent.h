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
};

}  // namespace ecs

