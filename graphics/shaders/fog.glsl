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

  // Integrate fog along the camera->pixel ray through the fog AABB.
  // This makes the fog feel volumetric (objects behind the volume get fogged too).

  vec3 toPos = worldPos - cameraPos;
  float totalDist = length(toPos);
  if (totalDist <= 1e-5) return 0.0;

  vec3 rd = toPos / totalDist;

  vec3 bmin = u_FogCenter - u_FogHalfSize;
  vec3 bmax = u_FogCenter + u_FogHalfSize;

  float tEnter = -1e20;
  float tExit = 1e20;

  // X slab
  if (abs(rd.x) < 1e-6) {
    if (cameraPos.x < bmin.x || cameraPos.x > bmax.x) return 0.0;
  } else {
    float t1 = (bmin.x - cameraPos.x) / rd.x;
    float t2 = (bmax.x - cameraPos.x) / rd.x;
    tEnter = max(tEnter, min(t1, t2));
    tExit = min(tExit, max(t1, t2));
  }
  // Y slab
  if (abs(rd.y) < 1e-6) {
    if (cameraPos.y < bmin.y || cameraPos.y > bmax.y) return 0.0;
  } else {
    float t1 = (bmin.y - cameraPos.y) / rd.y;
    float t2 = (bmax.y - cameraPos.y) / rd.y;
    tEnter = max(tEnter, min(t1, t2));
    tExit = min(tExit, max(t1, t2));
  }
  // Z slab
  if (abs(rd.z) < 1e-6) {
    if (cameraPos.z < bmin.z || cameraPos.z > bmax.z) return 0.0;
  } else {
    float t1 = (bmin.z - cameraPos.z) / rd.z;
    float t2 = (bmax.z - cameraPos.z) / rd.z;
    tEnter = max(tEnter, min(t1, t2));
    tExit = min(tExit, max(t1, t2));
  }

  if (tExit <= tEnter) return 0.0;

  // Clamp to the segment we actually shade (camera -> worldPos), plus artist start/end distances.
  float segStart = max(0.0, tEnter);
  float segEnd = min(totalDist, tExit);
  segStart = max(segStart, u_FogStart);
  segEnd = min(segEnd, min(totalDist, u_FogEnd));

  float segLen = max(0.0, segEnd - segStart);
  if (segLen <= 1e-5) return 0.0;

  // Exponential transmittance through the fog segment.
  float f = 1.0 - exp(-u_FogDensity * segLen);

  // Distance ramp so fog can be kept off near-camera but reach full by endDistance.
  float range = max(0.001, (u_FogEnd - u_FogStart));
  float ramp = clamp((segEnd - u_FogStart) / range, 0.0, 1.0);
  f *= ramp;

  // Height falloff (approx integrated by sampling the midpoint of the in-volume segment).
  vec3 midPos = cameraPos + rd * ((segStart + segEnd) * 0.5);
  float h = midPos.y - (u_FogCenter.y + u_FogBaseHeight);
  float hf = exp(-abs(h) * u_FogHeightFalloff);
  f *= hf;

  return clamp(f, 0.0, 1.0);
}

#endif
