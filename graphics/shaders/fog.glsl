// Shared atmospheric mist uniforms + helpers.
// Intended to be `#include`'d by other shaders.

#ifndef DUPPY_FOG_GLSL
#define DUPPY_FOG_GLSL 1

uniform int u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform float u_FogMaxOpacity;
uniform float u_FogDistanceExponent;
uniform float u_FogHeightFalloff;
uniform float u_FogBaseHeight;
uniform float u_FogHorizonStrength;
uniform float u_FogNoiseScale;
uniform float u_FogNoiseStrength;
uniform float u_FogDetailNoiseScale;
uniform float u_FogDetailNoiseStrength;
uniform vec2 u_FogWindDirection;
uniform float u_FogWindSpeed;
uniform float u_FogTime;
uniform vec2 u_FogNoiseOffset;

float fogSaturate(float x) { return clamp(x, 0.0, 1.0); }

float fogHash12(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float fogValueNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  float a = fogHash12(i);
  float b = fogHash12(i + vec2(1.0, 0.0));
  float c = fogHash12(i + vec2(0.0, 1.0));
  float d = fogHash12(i + vec2(1.0, 1.0));
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fogFbm(vec2 p) {
  float sum = 0.0;
  float amp = 0.5;
  float freq = 1.0;
  for (int i = 0; i < 5; ++i) {
    sum += amp * fogValueNoise(p * freq);
    freq *= 2.02;
    amp *= 0.5;
  }
  return sum;
}

vec2 fogSafeDir(vec2 v) {
  float lenSq = dot(v, v);
  if (lenSq <= 1e-6) return vec2(1.0, 0.0);
  return v * inversesqrt(lenSq);
}

float fogDensityAt(vec3 worldPos) {
  float heightMeters = max(0.0, worldPos.y - u_FogBaseHeight);
  float heightTerm = exp(-heightMeters * max(0.0001, u_FogHeightFalloff));

  vec2 wind = fogSafeDir(u_FogWindDirection);
  vec2 xz = worldPos.xz - u_FogNoiseOffset;
  vec2 macroUv = xz * max(0.0001, u_FogNoiseScale) + wind * (u_FogTime * u_FogWindSpeed * 0.020);
  vec2 detailUv = xz * max(0.0001, u_FogDetailNoiseScale) - wind * (u_FogTime * u_FogWindSpeed * 0.050);

  float macro = mix(1.0, mix(0.55, 1.45, fogFbm(macroUv)), fogSaturate(u_FogNoiseStrength));
  float detail = mix(1.0, mix(0.76, 1.24, fogFbm(detailUv + vec2(11.3, 3.7))), fogSaturate(u_FogDetailNoiseStrength));
  return max(0.0, u_FogDensity * heightTerm * macro * detail);
}

float fogFactorAt(vec3 cameraPos, vec3 worldPos) {
  if (u_FogEnabled == 0) return 0.0;

  vec3 toPos = worldPos - cameraPos;
  float totalDist = length(toPos);
  if (totalDist <= 1e-5) return 0.0;

  vec3 rd = toPos / totalDist;
  float segStart = max(0.0, u_FogStart);
  float segEnd = min(totalDist, u_FogEnd);
  if (segEnd <= segStart + 1e-5) return 0.0;

  const int sampleCount = 6;
  float segLen = segEnd - segStart;
  float stepLen = segLen / float(sampleCount);
  float opticalDepth = 0.0;

  for (int i = 0; i < sampleCount; ++i) {
    float t = (float(i) + 0.5) / float(sampleCount);
    vec3 samplePos = cameraPos + rd * mix(segStart, segEnd, t);
    opticalDepth += fogDensityAt(samplePos) * stepLen;
  }

  float range = max(0.001, u_FogEnd - u_FogStart);
  float ramp = pow(fogSaturate((segEnd - u_FogStart) / range), max(0.35, u_FogDistanceExponent));
  float horizon = 1.0 + fogSaturate(1.0 - abs(rd.y)) * max(0.0, u_FogHorizonStrength);
  float fog = (1.0 - exp(-opticalDepth * horizon)) * ramp;
  return clamp(fog, 0.0, clamp(u_FogMaxOpacity, 0.0, 1.0));
}

#endif
