#version 410 core

#include "shadow.glsl"

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_Uv;

out vec4 o_Color;

uniform vec3 u_CameraPos;
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;
uniform vec3 u_BaseColor;

uniform bool u_UseAlbedo;
uniform sampler2D u_AlbedoTex;
uniform bool u_UseNormal;
uniform sampler2D u_NormalTex;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  vec3 N = normalize(v_WorldNormal);
  if (u_UseNormal) {
    vec3 texN = texture(u_NormalTex, v_Uv).xyz * 2.0 - 1.0;
    N = normalize(N + vec3(texN.x, texN.y, texN.z) * 0.22);
  }

  vec3 base = u_BaseColor;
  if (u_UseAlbedo) {
    base *= texture(u_AlbedoTex, v_Uv).rgb;
  }

  vec3 L = normalize(-u_SunDir);
  vec3 V = normalize(u_CameraPos - v_WorldPos);
  float ndl = saturate(dot(N, L));
  float lit = shadowVisibility(v_WorldPos, N, u_SunDir);

  vec3 H = normalize(L + V);
  float spec = pow(saturate(dot(N, H)), 48.0) * 0.08;
  vec3 ambient = base * 0.18;
  vec3 direct = (base * ndl + spec) * u_SunColor * u_SunIntensity * lit;
  vec3 color = ambient + direct;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, 1.0);
}
