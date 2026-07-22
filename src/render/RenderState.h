#pragma once

// Author: Karl-Johan Bailey
//
// RenderState (descriptive)
// Small enums describing common render pipeline state.

namespace render {

enum class RenderMode {
  Opaque,
  Masked,
  Transparent,
  Additive,
};

enum class CullMode {
  Back,
  Front,
  None,
};

enum class DepthTest {
  Disabled,
  Less,
  LessEqual,
  Equal,
  Greater,
  Always,
};

enum class BlendMode {
  Disabled,
  Alpha,
  PremultipliedAlpha,
  Additive,
  Multiply,
};

}  // namespace render

