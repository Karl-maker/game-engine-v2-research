#pragma once

// Author: Karl-Johan Bailey
//
// JsonUtil
// Small helpers for reading typed values from JsonValue objects.

#include "data/Json.h"
#include "math/Vec3.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace data {

inline bool readBool(const JsonValue& v, bool& out) {
  if (const auto* b = v.tryBool()) {
    out = *b;
    return true;
  }
  return false;
}

inline bool readDouble(const JsonValue& v, double& out) {
  if (const auto* n = v.tryNumber()) {
    out = *n;
    return true;
  }
  return false;
}

inline bool readFloat(const JsonValue& v, float& out) {
  double d = 0.0;
  if (!readDouble(v, d)) return false;
  out = static_cast<float>(d);
  return true;
}

inline bool readInt(const JsonValue& v, int& out) {
  double d = 0.0;
  if (!readDouble(v, d)) return false;
  out = static_cast<int>(d);
  return true;
}

inline bool readString(const JsonValue& v, std::string& out) {
  if (const auto* s = v.tryString()) {
    out = *s;
    return true;
  }
  return false;
}

inline const JsonValue* objectKey(const JsonValue& v, const char* key) {
  const auto* o = v.tryObject();
  if (!o) return nullptr;
  return getObjectKey(*o, key);
}

inline bool readVec3(const JsonValue& v, math::Vec3& out) {
  if (const auto* a = v.tryArray()) {
    if (a->size() < 3) return false;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!readFloat((*a)[0], x) || !readFloat((*a)[1], y) || !readFloat((*a)[2], z)) return false;
    out = {x, y, z};
    return true;
  }
  if (const auto* o = v.tryObject()) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    const JsonValue* jx = getObjectKey(*o, "x");
    const JsonValue* jy = getObjectKey(*o, "y");
    const JsonValue* jz = getObjectKey(*o, "z");
    if (!jx || !jy || !jz) return false;
    if (!readFloat(*jx, x) || !readFloat(*jy, y) || !readFloat(*jz, z)) return false;
    out = {x, y, z};
    return true;
  }
  return false;
}

inline float getFloatOr(const JsonValue::Object& obj, const char* key, float fallback) {
  if (const auto* v = getObjectKey(obj, key)) {
    float f = fallback;
    if (readFloat(*v, f)) return f;
  }
  return fallback;
}

inline int getIntOr(const JsonValue::Object& obj, const char* key, int fallback) {
  if (const auto* v = getObjectKey(obj, key)) {
    int n = fallback;
    if (readInt(*v, n)) return n;
  }
  return fallback;
}

inline bool getBoolOr(const JsonValue::Object& obj, const char* key, bool fallback) {
  if (const auto* v = getObjectKey(obj, key)) {
    bool b = fallback;
    if (readBool(*v, b)) return b;
  }
  return fallback;
}

inline std::string getStringOr(const JsonValue::Object& obj, const char* key, std::string fallback) {
  if (const auto* v = getObjectKey(obj, key)) {
    std::string s;
    if (readString(*v, s)) return s;
  }
  return fallback;
}

}  // namespace data

