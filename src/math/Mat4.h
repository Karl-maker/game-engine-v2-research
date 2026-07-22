#pragma once

// Author: Karl-Johan Bailey
//
// Mat4 (minimal)
// Column-major 4x4 matrix helpers intended for simple rendering.

#include "math/Vec3.h"

#include <cmath>

namespace math {

struct Mat4 {
  // Column-major storage: m[col*4 + row]
  float m[16] = {
      1, 0, 0, 0,  //
      0, 1, 0, 0,  //
      0, 0, 1, 0,  //
      0, 0, 0, 1,  //
  };
};

inline Mat4 identity() { return {}; }

inline Mat4 mul(const Mat4& a, const Mat4& b) {
  Mat4 out{};
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      out.m[c * 4 + r] = a.m[0 * 4 + r] * b.m[c * 4 + 0] + a.m[1 * 4 + r] * b.m[c * 4 + 1] +
                         a.m[2 * 4 + r] * b.m[c * 4 + 2] + a.m[3 * 4 + r] * b.m[c * 4 + 3];
    }
  }
  return out;
}

inline Mat4 translate(const Vec3& t) {
  Mat4 out = identity();
  out.m[12] = t.x;
  out.m[13] = t.y;
  out.m[14] = t.z;
  return out;
}

inline Mat4 scale(const Vec3& s) {
  Mat4 out = identity();
  out.m[0] = s.x;
  out.m[5] = s.y;
  out.m[10] = s.z;
  return out;
}

inline Mat4 rotateX(float radians) {
  Mat4 out = identity();
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  out.m[5] = c;
  out.m[6] = s;
  out.m[9] = -s;
  out.m[10] = c;
  return out;
}

inline Mat4 rotateY(float radians) {
  Mat4 out = identity();
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  out.m[0] = c;
  out.m[2] = -s;
  out.m[8] = s;
  out.m[10] = c;
  return out;
}

inline Mat4 rotateZ(float radians) {
  Mat4 out = identity();
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  out.m[0] = c;
  out.m[1] = s;
  out.m[4] = -s;
  out.m[5] = c;
  return out;
}

inline Mat4 perspective(float fovYRadians, float aspect, float zNear, float zFar) {
  const float f = 1.0f / std::tan(fovYRadians * 0.5f);
  Mat4 out{};
  for (float& v : out.m) v = 0.0f;
  out.m[0] = f / aspect;
  out.m[5] = f;
  out.m[10] = (zFar + zNear) / (zNear - zFar);
  out.m[11] = -1.0f;
  out.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
  return out;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
  return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
  const Vec3 f = normalize(center - eye);
  const Vec3 s = normalize(cross(f, up));
  const Vec3 u = cross(s, f);

  Mat4 out = identity();
  out.m[0] = s.x;
  out.m[4] = s.y;
  out.m[8] = s.z;

  out.m[1] = u.x;
  out.m[5] = u.y;
  out.m[9] = u.z;

  out.m[2] = -f.x;
  out.m[6] = -f.y;
  out.m[10] = -f.z;

  out.m[12] = -dot(s, eye);
  out.m[13] = -dot(u, eye);
  out.m[14] = dot(f, eye);
  return out;
}

}  // namespace math

