// Instanced rocks fragment shader (simple)

#version 410 core

#include "fog.glsl"

in vec3 v_WorldPos;
in vec3 v_WorldNormal;

out vec4 o_Color;

uniform vec3 u_CameraPos;
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  vec3 N = normalize(v_WorldNormal);
  vec3 V = normalize(u_CameraPos - v_WorldPos);
  vec3 L = normalize(-u_SunDir);
  float ndl = saturate(dot(N, L));

  // Simple rock color.
  vec3 base = vec3(0.36, 0.34, 0.30);
  base *= 0.85 + ndl * 0.35;

  // Tiny spec highlight.
  vec3 H = normalize(L + V);
  float ndh = saturate(dot(N, H));
  float spec = pow(ndh, 32.0) * 0.08;

  vec3 light = u_SunColor * u_SunIntensity;
  vec3 color = base * (0.20 + 0.80 * ndl) * light + spec * light;
  color = mix(color, u_FogColor, fogFactorAt(u_CameraPos, v_WorldPos));

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, 1.0);
}
