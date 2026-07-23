#pragma once

// Author: Karl-Johan Bailey
//
// Mat4 (minimal)
// Column-major 4x4 matrix helpers intended for simple rendering.

#include "math/Vec3.h"

#include <algorithm>
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

struct Quat {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 1.0f;
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

inline Quat normalize(const Quat& q) {
  const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (len <= 0.000001f) return {};
  const float inv = 1.0f / len;
  return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}

inline Quat slerp(const Quat& a, const Quat& b, float t) {
  Quat qb = b;
  float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  if (d < 0.0f) {
    qb = {-b.x, -b.y, -b.z, -b.w};
    d = -d;
  }
  if (d > 0.9995f) {
    return normalize({a.x + (qb.x - a.x) * t, a.y + (qb.y - a.y) * t, a.z + (qb.z - a.z) * t, a.w + (qb.w - a.w) * t});
  }
  const float theta0 = std::acos(std::max(-1.0f, std::min(1.0f, d)));
  const float theta = theta0 * t;
  const float sinTheta = std::sin(theta);
  const float sinTheta0 = std::sin(theta0);
  const float s0 = std::cos(theta) - d * sinTheta / sinTheta0;
  const float s1 = sinTheta / sinTheta0;
  return {a.x * s0 + qb.x * s1, a.y * s0 + qb.y * s1, a.z * s0 + qb.z * s1, a.w * s0 + qb.w * s1};
}

inline Mat4 rotate(const Quat& qIn) {
  const Quat q = normalize(qIn);
  Mat4 out = identity();
  const float xx = q.x * q.x;
  const float yy = q.y * q.y;
  const float zz = q.z * q.z;
  const float xy = q.x * q.y;
  const float xz = q.x * q.z;
  const float yz = q.y * q.z;
  const float wx = q.w * q.x;
  const float wy = q.w * q.y;
  const float wz = q.w * q.z;
  out.m[0] = 1.0f - 2.0f * (yy + zz);
  out.m[1] = 2.0f * (xy + wz);
  out.m[2] = 2.0f * (xz - wy);
  out.m[4] = 2.0f * (xy - wz);
  out.m[5] = 1.0f - 2.0f * (xx + zz);
  out.m[6] = 2.0f * (yz + wx);
  out.m[8] = 2.0f * (xz + wy);
  out.m[9] = 2.0f * (yz - wx);
  out.m[10] = 1.0f - 2.0f * (xx + yy);
  return out;
}

inline Mat4 compose(const Vec3& translation, const Quat& rotation, const Vec3& scaleValue) {
  return mul(mul(translate(translation), rotate(rotation)), scale(scaleValue));
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

inline Mat4 inverseAffine(const Mat4& m) {
  const float a00 = m.m[0], a01 = m.m[4], a02 = m.m[8];
  const float a10 = m.m[1], a11 = m.m[5], a12 = m.m[9];
  const float a20 = m.m[2], a21 = m.m[6], a22 = m.m[10];
  const float det = a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
  if (std::fabs(det) <= 1e-8f) {
    return identity();
  }
  const float invDet = 1.0f / det;

  Mat4 out{};
  out.m[0] = (a11 * a22 - a12 * a21) * invDet;
  out.m[1] = (a12 * a20 - a10 * a22) * invDet;
  out.m[2] = (a10 * a21 - a11 * a20) * invDet;
  out.m[3] = 0.0f;

  out.m[4] = (a02 * a21 - a01 * a22) * invDet;
  out.m[5] = (a00 * a22 - a02 * a20) * invDet;
  out.m[6] = (a01 * a20 - a00 * a21) * invDet;
  out.m[7] = 0.0f;

  out.m[8] = (a01 * a12 - a02 * a11) * invDet;
  out.m[9] = (a02 * a10 - a00 * a12) * invDet;
  out.m[10] = (a00 * a11 - a01 * a10) * invDet;
  out.m[11] = 0.0f;

  const Vec3 t{m.m[12], m.m[13], m.m[14]};
  out.m[12] = -(out.m[0] * t.x + out.m[4] * t.y + out.m[8] * t.z);
  out.m[13] = -(out.m[1] * t.x + out.m[5] * t.y + out.m[9] * t.z);
  out.m[14] = -(out.m[2] * t.x + out.m[6] * t.y + out.m[10] * t.z);
  out.m[15] = 1.0f;
  return out;
}

}  // namespace math
