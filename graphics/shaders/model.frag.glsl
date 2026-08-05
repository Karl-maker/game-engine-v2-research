#version 410 core

#include "fog.glsl"
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
uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_SpecularIntensity;
uniform float u_NormalStrength;
uniform float u_AOStrength;
uniform vec3 u_EmissiveColor;
uniform float u_EmissiveStrength;
uniform float u_DisplacementStrength;

uniform bool u_UseAlbedo;
uniform sampler2D u_AlbedoTex;
uniform bool u_UseNormal;
uniform sampler2D u_NormalTex;
uniform bool u_UseRoughness;
uniform sampler2D u_RoughnessTex;
uniform bool u_UseMetallic;
uniform sampler2D u_MetallicTex;
uniform bool u_UseAO;
uniform sampler2D u_AOTex;
uniform bool u_UseSpecular;
uniform sampler2D u_SpecularTex;
uniform bool u_UseEmissive;
uniform sampler2D u_EmissiveTex;
uniform bool u_UseDisplacement;
uniform sampler2D u_DisplacementTex;
uniform bool u_UseMetallicRoughness;
uniform sampler2D u_MetallicRoughnessTex;
uniform bool u_UseOrm;
uniform sampler2D u_OrmTex;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  vec3 N = normalize(v_WorldNormal);
  if (u_UseNormal) {
    vec3 texN = texture(u_NormalTex, v_Uv).xyz * 2.0 - 1.0;
    N = normalize(N + vec3(texN.x, texN.y, texN.z) * max(0.0, u_NormalStrength));
  }

  vec3 base = u_BaseColor;
  if (u_UseAlbedo) {
    base *= texture(u_AlbedoTex, v_Uv).rgb;
  }

  float roughness = clamp(u_Roughness, 0.04, 1.0);
  float metallic = clamp(u_Metallic, 0.0, 1.0);
  float ao = max(0.0, u_AOStrength);
  float specularScale = max(0.0, u_SpecularIntensity);
  vec3 emissive = u_EmissiveColor * max(0.0, u_EmissiveStrength);

  if (u_UseMetallicRoughness) {
    vec3 mr = texture(u_MetallicRoughnessTex, v_Uv).rgb;
    roughness = clamp(roughness * mr.g, 0.04, 1.0);
    metallic = clamp(metallic * mr.b, 0.0, 1.0);
  }

  if (u_UseOrm) {
    vec3 orm = texture(u_OrmTex, v_Uv).rgb;
    ao *= orm.r;
    roughness = clamp(roughness * orm.g, 0.04, 1.0);
    metallic = clamp(metallic * orm.b, 0.0, 1.0);
  }

  if (u_UseRoughness) {
    roughness = clamp(roughness * texture(u_RoughnessTex, v_Uv).r, 0.04, 1.0);
  }
  if (u_UseMetallic) {
    metallic = clamp(metallic * texture(u_MetallicTex, v_Uv).r, 0.0, 1.0);
  }
  if (u_UseAO) {
    ao *= texture(u_AOTex, v_Uv).r;
  }
  if (u_UseSpecular) {
    specularScale *= texture(u_SpecularTex, v_Uv).r;
  }
  if (u_UseEmissive) {
    emissive += texture(u_EmissiveTex, v_Uv).rgb * max(0.0, u_EmissiveStrength);
  }
  if (u_UseDisplacement) {
    float h = texture(u_DisplacementTex, v_Uv).r - 0.5;
    base *= 1.0 + h * (0.20 * u_DisplacementStrength);
    roughness = clamp(roughness + (0.5 - h) * (0.20 * u_DisplacementStrength), 0.04, 1.0);
  }

  vec3 L = normalize(-u_SunDir);
  vec3 V = normalize(u_CameraPos - v_WorldPos);
  float ndl = saturate(dot(N, L));
  float lit = shadowVisibility(v_WorldPos, N, u_SunDir);

  vec3 H = normalize(L + V);
  float shininess = mix(96.0, 6.0, roughness);
  float spec = pow(saturate(dot(N, H)), shininess) * mix(1.25, 0.18, roughness) * specularScale;
  vec3 diffuse = base * (1.0 - metallic);
  vec3 ambient = diffuse * (0.12 + 0.12 * (1.0 - roughness)) * ao;
  vec3 specColor = mix(vec3(0.04), base, metallic) * spec;
  vec3 direct = (diffuse * ndl + specColor) * u_SunColor * u_SunIntensity * lit;
  vec3 color = ambient + direct + emissive;
  color = mix(color, u_FogColor, fogFactorAt(u_CameraPos, v_WorldPos));

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, 1.0);
}
