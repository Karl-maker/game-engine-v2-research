// Shared shadow map uniforms + helpers (demo)
// Intended to be `#include`'d by other shaders.

#ifndef DUPPY_SHADOW_GLSL
#define DUPPY_SHADOW_GLSL 1

uniform int u_ShadowEnabled;
uniform mat4 u_LightViewProj;
uniform sampler2DShadow u_ShadowMap;
uniform vec2 u_ShadowTexelSize;
uniform float u_ShadowBias;
uniform float u_ShadowStrength;

float shadowVisibility(vec3 worldPos, vec3 normal, vec3 lightDir) {
  if (u_ShadowEnabled == 0) return 1.0;

  vec4 ls = u_LightViewProj * vec4(worldPos, 1.0);
  vec3 ndc = ls.xyz / max(1e-6, ls.w);
  vec2 uv = ndc.xy * 0.5 + 0.5;
  float depth = ndc.z * 0.5 + 0.5;

  // Outside shadow map.
  if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;
  if (depth < 0.0 || depth > 1.0) return 1.0;

  float ndl = clamp(dot(normalize(normal), normalize(-lightDir)), 0.0, 1.0);
  float bias = u_ShadowBias * (1.0 - ndl);

  // 3x3 PCF.
  float vis = 0.0;
  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      vec2 o = vec2(float(x), float(y)) * u_ShadowTexelSize;
      vis += texture(u_ShadowMap, vec3(uv + o, depth - bias));
    }
  }
  vis *= (1.0 / 9.0);

  // Strength allows fading shadows out without changing lighting.
  return mix(1.0, vis, clamp(u_ShadowStrength, 0.0, 1.0));
}

#endif

