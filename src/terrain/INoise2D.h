#pragma once

// Author: Karl-Johan Bailey
//
// INoise2D
// Runtime noise service interface. Useful when you want real height sampling at runtime
// or for generating heightmaps.

namespace terrain {

class INoise2D {
 public:
  virtual ~INoise2D() = default;
  virtual float sample(float x, float y) const = 0;  // return typically in [-1, 1]
};

}  // namespace terrain

