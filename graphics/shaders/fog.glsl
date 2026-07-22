// Shared fog uniforms + helpers (demo)
// Intended to be `#include`'d by other shaders.

#ifndef DUPPY_FOG_GLSL
#define DUPPY_FOG_GLSL 1

uniform int u_FogEnabled;
uniform vec3 u_FogCenter;
uniform vec3 u_FogHalfSize;
uniform vec3 u_FogColor;
uniform float u_FogDensity;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform float u_FogHeightFalloff;
uniform float u_FogBaseHeight;

float fogFactorAt(vec3 cameraPos, vec3 worldPos) {
  if (u_FogEnabled == 0) return 0.0;

  vec3 d = abs(worldPos - u_FogCenter);
  if (d.x > u_FogHalfSize.x || d.y > u_FogHalfSize.y || d.z > u_FogHalfSize.z) return 0.0;

  float dist = length(cameraPos - worldPos);
  float distFromStart = max(0.0, dist - u_FogStart);
  float range = max(0.001, (u_FogEnd - u_FogStart));

  // Stronger near-camera fog: exponential with an additional distance ramp to reach full by endDistance.
  float expFog = 1.0 - exp(-u_FogDensity * distFromStart);
  float ramp = clamp(distFromStart / range, 0.0, 1.0);
  float f = expFog * ramp;

  // Height falloff (stronger near base height).
  float h = worldPos.y - (u_FogCenter.y + u_FogBaseHeight);
  float hf = exp(-abs(h) * u_FogHeightFalloff);
  f *= hf;

  return clamp(f, 0.0, 1.0);
}

#endif

