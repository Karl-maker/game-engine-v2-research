#pragma once

// Author: Karl-Johan Bailey
//
// MaterialParameter (descriptive)
// Named, typed parameters for shaders/materials.

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"
#include "render/Color.h"

#include <string>
#include <variant>

namespace render {

using MaterialParamValue = std::variant<bool, int, float, math::Vec2, math::Vec3, math::Vec4, Color>;

struct MaterialParameter {
  std::string name;
  MaterialParamValue value;
};

}  // namespace render

