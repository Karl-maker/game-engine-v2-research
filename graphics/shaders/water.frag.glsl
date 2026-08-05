#version 410 core

#include "fog.glsl"

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_Uv;

out vec4 o_Color;

uniform vec3 u_CameraPos;
uniform float u_Time;

uniform vec4 u_BaseColor = vec4(0.03, 0.18, 0.24, 1.0);
uniform vec3 u_ShallowColor = vec3(0.18, 0.46, 0.52);
uniform vec3 u_FoamColor = vec3(0.92, 0.96, 0.98);
uniform float u_Roughness = 0.14;
uniform float u_Metallic = 0.0;
uniform float u_SpecularIntensity = 0.72;
uniform float u_NormalStrength = 0.22;
uniform float u_WaterAlpha = 0.72;
uniform float u_Clarity = 0.72;
uniform float u_ShoreFadeDistance = 6.0;
uniform float u_ShoreFoamDepth = 1.15;
uniform float u_ShoreFoamStrength = 0.22;
uniform float u_ShoreTerrainBaseY = 0.0;
uniform float u_ShoreTerrainHeightScale = 0.0;
uniform float u_HeightMapStrength = 1.0;
uniform int u_HeightMapInvert = 0;
uniform float u_WaterLevel = 0.0;

uniform float u_WaveHeight = 0.0;
uniform float u_WaveScale = 0.085;
uniform float u_WaveSpeed = 0.28;
uniform vec2 u_WaveDirection = vec2(1.0, 0.2);
uniform float u_SecondaryWaveHeight = 0.0;
uniform float u_SecondaryWaveScale = 0.16;
uniform float u_SecondaryWaveSpeed = 0.18;
uniform vec2 u_SecondaryWaveDirection = vec2(-0.35, 1.0);

uniform float u_RippleTiling = 0.12;
uniform float u_RippleStrength = 0.10;
uniform float u_FoamTiling = 0.085;
uniform float u_FoamStrength = 0.12;
uniform vec2 u_MapUvTiling = vec2(1.0);

uniform sampler2D u_NormalTex;
uniform bool u_UseNormal = false;
uniform sampler2D u_HeightMapTex;
uniform bool u_UseHeightMap = false;
uniform sampler2D u_FoamNormalTex;
uniform bool u_UseFoamNormal = false;
uniform sampler2D u_RippleMaskTex;
uniform bool u_UseRippleMask = false;

const int MAX_LIGHTS = 16;
uniform int u_LightCount;
uniform int u_LightType[MAX_LIGHTS];
uniform vec3 u_LightPos[MAX_LIGHTS];
uniform vec3 u_LightDir[MAX_LIGHTS];
uniform vec3 u_LightColor[MAX_LIGHTS];
uniform float u_LightIntensity[MAX_LIGHTS];
uniform float u_LightRange[MAX_LIGHTS];

float saturate(float x) { return clamp(x, 0.0, 1.0); }

vec2 safeDir(vec2 v) {
  float lenSq = dot(v, v);
  if (lenSq < 1e-5) return vec2(1.0, 0.0);
  return v * inversesqrt(lenSq);
}

vec3 tangentNormal(sampler2D tex, vec2 uv) {
  vec3 n = texture(tex, uv).xyz * 2.0 - 1.0;
  return normalize(vec3(n.x, max(0.02, n.z), n.y));
}

vec3 applyPlanarNormal(vec3 baseN, vec3 tangentN) {
  vec3 T = vec3(1.0, 0.0, 0.0);
  vec3 B = vec3(0.0, 0.0, 1.0);
  return normalize(T * tangentN.x + baseN * tangentN.y + B * tangentN.z);
}

float rippleHeight(vec2 uv) {
  if (!u_UseRippleMask) return 0.0;
  return texture(u_RippleMaskTex, uv).r;
}

vec3 rippleNormal(vec2 uv) {
  if (!u_UseRippleMask) return vec3(0.0, 1.0, 0.0);
  float eps = 0.003;
  float h = rippleHeight(uv);
  float hx = rippleHeight(uv + vec2(eps, 0.0));
  float hy = rippleHeight(uv + vec2(0.0, eps));
  vec2 grad = vec2(hx - h, hy - h) / eps;
  return normalize(vec3(-grad.x, 1.0 / max(0.001, u_RippleStrength * 12.0), -grad.y));
}

vec2 waveGradient(vec2 worldXZ) {
  vec2 dirA = safeDir(u_WaveDirection);
  vec2 dirB = safeDir(u_SecondaryWaveDirection);
  float phaseA = dot(worldXZ, dirA) * u_WaveScale + u_Time * u_WaveSpeed;
  float phaseB = dot(worldXZ, dirB) * u_SecondaryWaveScale + u_Time * u_SecondaryWaveSpeed;
  vec2 grad = cos(phaseA) * u_WaveHeight * u_WaveScale * dirA;
  grad += cos(phaseB) * u_SecondaryWaveHeight * u_SecondaryWaveScale * dirB;
  return grad;
}

float terrainHeightAt(vec2 uv) {
  if (!u_UseHeightMap || u_ShoreTerrainHeightScale <= 0.0) return u_ShoreTerrainBaseY;
  float h = texture(u_HeightMapTex, uv * u_MapUvTiling).r;
  if (u_HeightMapInvert != 0) h = 1.0 - h;
  return u_ShoreTerrainBaseY + h * u_ShoreTerrainHeightScale * u_HeightMapStrength;
}

void main() {
  vec3 V = normalize(u_CameraPos - v_WorldPos);
  vec2 grad = waveGradient(v_WorldPos.xz);
  vec3 N = normalize(vec3(-grad.x, 1.0, -grad.y));

  if (u_UseNormal) {
    vec2 uvA = v_WorldPos.xz * 0.045 + safeDir(u_WaveDirection) * (u_Time * 0.025);
    vec2 uvB = v_WorldPos.xz * 0.031 - safeDir(u_SecondaryWaveDirection) * (u_Time * 0.018);
    vec3 nA = tangentNormal(u_NormalTex, uvA);
    vec3 nB = tangentNormal(u_NormalTex, uvB);
    vec3 waterNormal = normalize(mix(nA, nB, 0.5));
    N = normalize(mix(N, applyPlanarNormal(N, waterNormal), saturate(u_NormalStrength)));
  }

  vec2 rippleUv = v_WorldPos.xz * u_RippleTiling + vec2(u_Time * 0.018, -u_Time * 0.012);
  N = normalize(mix(N, applyPlanarNormal(N, rippleNormal(rippleUv)), saturate(u_RippleStrength)));

  float groundHeight = terrainHeightAt(v_Uv);
  float waterDepth = max(0.0, u_WaterLevel - groundHeight);
  float deepness = saturate(waterDepth / max(0.001, u_ShoreFadeDistance));
  float shallowMask = 1.0 - deepness;

  vec3 waterColor = mix(u_ShallowColor, u_BaseColor.rgb, deepness);

  float rippleBreakup = u_UseRippleMask ? texture(u_RippleMaskTex, rippleUv).r : 0.5;
  float shoreMask = 1.0 - smoothstep(0.0, max(0.001, u_ShoreFoamDepth), waterDepth);
  float foam = shoreMask * mix(0.35, 1.0, rippleBreakup) * u_ShoreFoamStrength;

  if (u_UseFoamNormal) {
    vec2 foamUv = v_WorldPos.xz * u_FoamTiling + vec2(-u_Time * 0.015, u_Time * 0.009);
    vec3 foamNormal = tangentNormal(u_FoamNormalTex, foamUv);
    float foamBreakup = saturate(length(foamNormal.xz));
    foam += shoreMask * foamBreakup * u_FoamStrength;
    N = normalize(mix(N, applyPlanarNormal(N, foamNormal), shoreMask * 0.25));
  }

  foam = saturate(foam);

  vec3 lit = waterColor * vec3(0.06, 0.08, 0.10);
  float NdotV = saturate(dot(N, V));
  float fresnel = pow(1.0 - NdotV, 5.0);

  for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; ++i) {
    vec3 L = vec3(0.0, 1.0, 0.0);
    float attenuation = 1.0;

    if (u_LightType[i] == 0) {
      L = normalize(-u_LightDir[i]);
    } else {
      vec3 toLight = u_LightPos[i] - v_WorldPos;
      float dist = length(toLight);
      if (dist > 1e-4) L = toLight / dist;
      float range = max(0.001, u_LightRange[i]);
      float falloff = saturate(1.0 - dist / range);
      attenuation = falloff * falloff;
    }

    float NdotL = saturate(dot(N, L));
    if (NdotL <= 0.0) continue;

    vec3 H = normalize(L + V);
    float specPower = mix(128.0, 18.0, saturate(u_Roughness));
    float spec = pow(saturate(dot(N, H)), specPower) * mix(0.04, 1.1, saturate(u_SpecularIntensity));
    vec3 lightCol = u_LightColor[i] * u_LightIntensity[i] * attenuation;
    lit += waterColor * lightCol * (NdotL * 0.18);
    lit += lightCol * spec * mix(0.35, 1.0, fresnel);
  }

  vec3 skyReflect = mix(vec3(0.06, 0.15, 0.20), vec3(0.48, 0.64, 0.76), pow(1.0 - saturate(V.y * 0.5 + 0.5), 2.0));
  vec3 finalColor = mix(lit, skyReflect, clamp(0.25 + fresnel * 0.65, 0.0, 0.9));
  finalColor = mix(finalColor, u_FoamColor, foam);

  float alpha = mix(0.10, u_WaterAlpha, deepness);
  alpha *= mix(1.0, 0.55, saturate(u_Clarity));
  alpha = max(alpha, foam * 0.45);

  float fog = fogFactorAt(u_CameraPos, v_WorldPos);
  finalColor = mix(finalColor, u_FogColor, fog);
  o_Color = vec4(finalColor, alpha);
}
