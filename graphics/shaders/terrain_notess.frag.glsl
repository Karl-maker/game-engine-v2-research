// Terrain fragment shader (demo)
//
// Non-tessellated variant used for LOD/far rendering.
// Kept identical to `graphics/shaders/terrain.frag.glsl` so materials behave the same.

#version 410 core

#include "fog.glsl"

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_Uv;

out vec4 o_Color;

uniform vec3 u_CameraPos;
uniform float u_Time;

// Material-ish controls (example names).
uniform vec4 u_BaseColor;
uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_SpecularIntensity;
uniform float u_DirtColorNoiseStrength;

// Far grass tint (cheap distant "field" look)
uniform int u_GrassTintEnabled;
uniform vec3 u_GrassTintColor;
uniform float u_GrassTintNear;
uniform float u_GrassTintFar;
uniform float u_GrassTintStrength;

// Optional albedo texture.
uniform sampler2D u_Albedo;
uniform bool u_UseAlbedo = false;

uniform sampler2D u_NormalTex;
uniform bool u_UseNormal = false;
uniform float u_NormalStrength;
uniform sampler2D u_RoughnessTex;
uniform bool u_UseRoughness = false;
uniform int u_RoughnessInvert = 0;
uniform sampler2D u_AOTex;
uniform bool u_UseAO = false;
uniform float u_AOStrength;
uniform sampler2D u_DisplacementTex;
uniform bool u_UseDisplacement = false;
uniform float u_DisplacementStrength;
uniform int u_DisplacementInvert = 0;

uniform vec2 u_UvTiling;

// Rock layer (optional)
uniform int u_RockLayerEnabled;
uniform sampler2D u_RockAlbedo;
uniform bool u_UseRockAlbedo;
uniform sampler2D u_RockNormalTex;
uniform bool u_UseRockNormal;
uniform sampler2D u_RockRoughnessTex;
uniform bool u_UseRockRoughness;
uniform sampler2D u_RockAOTex;
uniform bool u_UseRockAO;
uniform sampler2D u_RockDisplacementTex;
uniform bool u_UseRockDisplacement;
uniform vec2 u_RockUvTiling;
uniform float u_RockNormalStrength;
uniform float u_RockDisplacementStrength;
uniform float u_RockBlendStrength;
uniform float u_RockNoiseScale;

// Terrain tile maps (aligned to terrain UVs, optional).
uniform int u_TerrainLodStep = 1;
uniform vec2 u_MapUvTiling = vec2(1.0, 1.0);
uniform float u_MapMipBias = 0.0;
uniform float u_MapMipScale = 1.0;
uniform float u_MapMipMax = 8.0;

uniform sampler2D u_HeightMapTex;
uniform bool u_UseHeightMap = false;

uniform sampler2D u_TerrainNormalMapTex;
uniform bool u_UseTerrainNormalMap = false;
uniform float u_TerrainNormalMapStrength = 1.0;

uniform sampler2D u_TerrainRoughnessMapTex;
uniform bool u_UseTerrainRoughnessMap = false;
uniform float u_TerrainRoughnessMapStrength = 1.0;
uniform int u_TerrainRoughnessInvert = 0;

uniform sampler2D u_TerrainSurfaceMapTex;
uniform bool u_UseTerrainSurfaceMap = false;
uniform float u_TerrainSurfaceStrength = 0.0;

uniform sampler2D u_SplatMapTex;
uniform bool u_UseSplatMap = false;
uniform float u_SplatStrength = 1.0;  // 0=procedural, 1=splat override
uniform int u_SplatChannel = 0;        // 0=R,1=G,2=B,3=A

// Minimal packed light data.
// type: 0=Directional, 1=Point, 2=Spot (matches LightComponent::Type order in C++).
const int MAX_LIGHTS = 16;
uniform int u_LightCount;
uniform int u_LightType[MAX_LIGHTS];
uniform vec3 u_LightPos[MAX_LIGHTS];
uniform vec3 u_LightDir[MAX_LIGHTS];
uniform vec3 u_LightColor[MAX_LIGHTS];
uniform float u_LightIntensity[MAX_LIGHTS];
uniform float u_LightRange[MAX_LIGHTS];

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float hash12(vec2 p) {
  // Cheap hash in [0,1)
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  float a = hash12(i + vec2(0.0, 0.0));
  float b = hash12(i + vec2(1.0, 0.0));
  float c = hash12(i + vec2(0.0, 1.0));
  float d = hash12(i + vec2(1.0, 1.0));
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
  float sum = 0.0;
  float amp = 0.5;
  float freq = 1.0;
  for (int i = 0; i < 5; ++i) {
    sum += amp * valueNoise(p * freq);
    freq *= 2.02;
    amp *= 0.5;
  }
  return sum;
}

vec3 perturbNormal(vec3 worldPos, vec3 N, float strength) {
  // Tiny procedural "bump" from height noise on XZ plane.
  vec2 p = worldPos.xz;
  float eps = 0.25;
  float h = fbm(p * 1.5);
  float hx = fbm((p + vec2(eps, 0.0)) * 1.5);
  float hz = fbm((p + vec2(0.0, eps)) * 1.5);
  vec2 grad = vec2(hx - h, hz - h) / eps;

  vec3 up = vec3(0.0, 1.0, 0.0);
  vec3 T = normalize(cross(up, N));
  // Handle near-parallel with up.
  if (length(T) < 0.001) {
    T = normalize(cross(vec3(1.0, 0.0, 0.0), N));
  }
  vec3 B = normalize(cross(N, T));

  vec3 bumped = normalize(N + (T * grad.x + B * grad.y) * strength);
  return bumped;
}

vec3 applyNormalMapFrom(sampler2D tex, vec3 N, vec2 uv, float strength) {
  // Tangent space aligned to world XZ for demo purposes.
  vec3 t = normalize(vec3(1.0, 0.0, 0.0));
  vec3 b = normalize(vec3(0.0, 0.0, 1.0));
  vec3 nTex = texture(tex, uv).xyz * 2.0 - 1.0;
  nTex.xy *= max(0.0, strength);
  vec3 nWorld = normalize(t * nTex.x + b * nTex.y + N * nTex.z);
  return nWorld;
}

vec3 applyNormalMapFromLod(sampler2D tex, vec3 N, vec2 uv, float strength, float lod) {
  vec3 t = normalize(vec3(1.0, 0.0, 0.0));
  vec3 b = normalize(vec3(0.0, 0.0, 1.0));
  vec3 nTex = textureLod(tex, uv, lod).xyz * 2.0 - 1.0;
  nTex.xy *= max(0.0, strength);
  vec3 nWorld = normalize(t * nTex.x + b * nTex.y + N * nTex.z);
  return nWorld;
}

float terrainMapLod() {
  float step = max(1.0, float(u_TerrainLodStep));
  float lod = log2(step) * max(0.0, u_MapMipScale) + u_MapMipBias;
  return clamp(lod, 0.0, max(0.0, u_MapMipMax));
}

vec3 applyHeightBump(vec3 N, sampler2D tex, vec2 uv, float strength) {
  float eps = 0.0025;
  float h = texture(tex, uv).r;
  float hx = texture(tex, uv + vec2(eps, 0.0)).r;
  float hy = texture(tex, uv + vec2(0.0, eps)).r;
  vec2 grad = vec2(hx - h, hy - h) / eps;
  vec3 t = normalize(vec3(1.0, 0.0, 0.0));
  vec3 b = normalize(vec3(0.0, 0.0, 1.0));
  vec3 bump = normalize(N + (t * grad.x + b * grad.y) * strength);
  return bump;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
  return a2 / max(3.14159265 * denom * denom, 1e-6);
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return NdotV / max(NdotV * (1.0 - k) + k, 1e-6);
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
  float ggx1 = geometrySchlickGGX(NdotV, roughness);
  float ggx2 = geometrySchlickGGX(NdotL, roughness);
  return ggx1 * ggx2;
}

vec3 shadePbrish(vec3 albedo, vec3 normal, vec3 viewDir, float roughness, float metallic) {
  vec3 result = vec3(0.0);

  for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; ++i) {
    vec3 L = vec3(0.0);
    float attenuation = 1.0;

    if (u_LightType[i] == 0) {
      // Directional: direction points "from light" in world-space.
      L = normalize(-u_LightDir[i]);
    } else {
      // Point/spot: position is world-space.
      vec3 toLight = u_LightPos[i] - v_WorldPos;
      float dist = length(toLight);
      if (dist > 0.0001) {
        L = toLight / dist;
      }

      float range = max(0.001, u_LightRange[i]);
      float r = saturate(1.0 - (dist / range));
      attenuation = r * r;
    }

    vec3 H = normalize(L + viewDir);

    float NdotL = saturate(dot(normal, L));
    float NdotV = saturate(dot(normal, viewDir));
    float NdotH = saturate(dot(normal, H));
    float HdotV = saturate(dot(H, viewDir));

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(HdotV, F0);
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);

    vec3 numerator = D * G * F;
    float denom = max(4.0 * NdotV * NdotL, 1e-6);
    vec3 spec = numerator / denom;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 radiance = u_LightColor[i] * u_LightIntensity[i] * attenuation;
    vec3 diffuse = (kD * albedo) / 3.14159265;
    result += (diffuse + spec) * radiance * NdotL;
  }

  // Cheap ambient.
  vec3 ambient = vec3(0.08) * albedo;
  result += ambient;

  return result;
}

void main() {
  vec3 N = normalize(v_WorldNormal);
  vec3 V = normalize(u_CameraPos - v_WorldPos);

  vec2 uv = v_Uv * u_UvTiling;
  vec2 mapUv = v_Uv * u_MapUvTiling;
  float mapLod = terrainMapLod();

  // If an albedo texture is bound, let it fully own the base color (baseColor acts as a fallback).
  vec3 albedo = u_BaseColor.rgb;
  if (u_UseAlbedo) {
    albedo = texture(u_Albedo, uv).rgb;
  }

  float roughness = u_Roughness;
  float metallic = u_Metallic;

  // Large-scale terrain normal (heightmap-derived bake, etc).
  if (u_UseTerrainNormalMap) {
    N = applyNormalMapFromLod(u_TerrainNormalMapTex, N, mapUv, u_TerrainNormalMapStrength, mapLod);
  }

  // Micro-bump from procedural noise.
  N = perturbNormal(v_WorldPos, N, 0.35);

  if (u_UseNormal) {
    N = applyNormalMapFrom(u_NormalTex, N, uv, u_NormalStrength);
  }

  if (u_UseRoughness) {
    float rTex = texture(u_RoughnessTex, uv).r;
    if (u_RoughnessInvert != 0) rTex = 1.0 - rTex;
    roughness = clamp(roughness * rTex, 0.04, 1.0);
  }

  if (u_UseTerrainRoughnessMap) {
    float rMap = textureLod(u_TerrainRoughnessMapTex, mapUv, mapLod).r;
    if (u_TerrainRoughnessInvert != 0) rMap = 1.0 - rMap;
    float s = saturate(u_TerrainRoughnessMapStrength);
    roughness = clamp(roughness * mix(1.0, rMap, s), 0.04, 1.0);
  }

  if (u_UseTerrainSurfaceMap && abs(u_TerrainSurfaceStrength) > 0.0001) {
    float surface = textureLod(u_TerrainSurfaceMapTex, mapUv, mapLod).r;
    float d = (surface - 0.5) * u_TerrainSurfaceStrength;
    roughness = clamp(roughness + d, 0.04, 1.0);
    albedo *= (1.0 + d * 0.20);
  }

  float ao = 1.0;
  if (u_UseAO) {
    ao = texture(u_AOTex, uv).r;
  }

  if (u_UseDisplacement) {
    // Minimal displacement influence (shading-only; no vertex displacement).
    float h = texture(u_DisplacementTex, uv).r;
    if (u_DisplacementInvert != 0) h = 1.0 - h;
    albedo *= 0.92 + (h - 0.5) * (0.26 * u_DisplacementStrength);
    N = applyHeightBump(N, u_DisplacementTex, uv, 0.95 * u_DisplacementStrength);
  }

  // --- Optional rock layer blended with a noisy mask (no blocky tiles) ---
  if (u_RockLayerEnabled != 0) {
    // Slope helps rocks appear where terrain is steeper.
    float slope = 1.0 - saturate(dot(N, vec3(0.0, 1.0, 0.0)));

    // Noise mask in world space, with slight domain warp to break tiling.
    vec2 wp = v_WorldPos.xz * u_RockNoiseScale;
    vec2 warp = vec2(fbm(wp * 1.3), fbm(wp * 1.7)) - 0.5;
    wp += warp * 0.45;
    float n = fbm(wp * 2.2);
    float mask = saturate((n - 0.45) * 2.2);
    mask = saturate(mask + slope * 0.65);
    mask *= saturate(u_RockBlendStrength);

    // Optional splat-map override for the layer mask (e.g. red => mud).
    if (u_UseSplatMap) {
      vec4 s = textureLod(u_SplatMapTex, mapUv, mapLod);
      float ch = (u_SplatChannel == 0) ? s.r : (u_SplatChannel == 1) ? s.g : (u_SplatChannel == 2) ? s.b : s.a;
      mask = mix(mask, ch, saturate(u_SplatStrength));
      mask = saturate(mask);
    }

    vec2 uv2 = v_Uv * u_RockUvTiling;
    uv2 += (warp * 0.08);

    vec3 rockAlbedo = vec3(0.45);
    if (u_UseRockAlbedo) rockAlbedo = texture(u_RockAlbedo, uv2).rgb;

    float rockRough = roughness;
    if (u_UseRockRoughness) rockRough = clamp(rockRough * texture(u_RockRoughnessTex, uv2).r, 0.04, 1.0);

    float rockAo = 1.0;
    if (u_UseRockAO) rockAo = texture(u_RockAOTex, uv2).r;

    vec3 rockN = N;
    if (u_UseRockNormal) rockN = applyNormalMapFrom(u_RockNormalTex, N, uv2, u_RockNormalStrength);
    if (u_UseRockDisplacement) {
      rockN = applyHeightBump(rockN, u_RockDisplacementTex, uv2, 1.25 * u_RockDisplacementStrength);
      rockAlbedo *= 0.90 + (texture(u_RockDisplacementTex, uv2).r - 0.5) * (0.32 * u_RockDisplacementStrength);
    }

    // Blend into base.
    albedo = mix(albedo, rockAlbedo, mask);
    roughness = mix(roughness, rockRough, mask);
    ao = mix(ao, rockAo, mask);
    N = normalize(mix(N, rockN, mask));
  }

  vec3 color = shadePbrish(albedo, N, V, roughness, metallic);
  color *= mix(1.0, ao, clamp(u_AOStrength, 0.0, 1.0));

  // Fog in linear space (more visible than applying after tonemap/gamma).
  float fogF = fogFactorAt(u_CameraPos, v_WorldPos);
  color = mix(color, u_FogColor, fogF);

  // Simple tonemap-ish curve + gamma for display.
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, 1.0);
}
