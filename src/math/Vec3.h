#pragma once

// Author: Karl-Johan Bailey
//
// Minimal math helpers for ECS demo systems (no external deps).

#include <cmath>

namespace math {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(const Vec3& v, float s) { return {v.x * s, v.y * s, v.z * s}; }

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float lengthSq(const Vec3& v) { return dot(v, v); }
inline float length(const Vec3& v) { return std::sqrt(lengthSq(v)); }

inline Vec3 normalize(const Vec3& v) {
  const float len = length(v);
  if (len <= 0.000001f) return {0.0f, 0.0f, 0.0f};
  return v * (1.0f / len);
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a * (1.0f - t) + b * t; }

}  // namespace math

