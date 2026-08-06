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
uniform float u_FoamNoiseScale = 0.035;
uniform float u_FoamNoiseStrength = 0.0;
uniform float u_FoamDriftSpeed = 0.08;
uniform vec2 u_FoamDriftDirection = vec2(0.8, 0.35);
uniform vec2 u_MapUvTiling = vec2(1.0);

uniform sampler2D u_NormalTex;
uniform bool u_UseNormal = false;
uniform sampler2D u_HeightMapTex;
uniform bool u_UseHeightMap = false;
uniform sampler2D u_FoamNormalTex;
uniform bool u_UseFoamNormal = false;
uniform sampler2D u_RippleMaskTex;
uniform bool u_UseRippleMask = false;
const int MAX_LOCAL_RIPPLES = 3;
uniform int u_LocalRippleCount = 0;
uniform vec2 u_LocalRipplePos[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleRadius[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleLength[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleWidth[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleStrength[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleMagnitude[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleFrequency[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleSpeed[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleFalloff[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleTiling[MAX_LOCAL_RIPPLES];
uniform vec2 u_LocalRippleDirection[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleDriftSpeed[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleFoamBoost[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleNoiseScale[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleNoiseStrength[MAX_LOCAL_RIPPLES];
uniform float u_LocalRippleNoiseSpeed[MAX_LOCAL_RIPPLES];
uniform int u_LocalRippleUseTexture[MAX_LOCAL_RIPPLES];
uniform sampler2D u_LocalRippleTex0;
uniform sampler2D u_LocalRippleTex1;
uniform sampler2D u_LocalRippleTex2;

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
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = hash12(i + vec2(0.0, 0.0));
  float b = hash12(i + vec2(1.0, 0.0));
  float c = hash12(i + vec2(0.0, 1.0));
  float d = hash12(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
  float sum = 0.0;
  float amp = 0.5;
  float freq = 1.0;
  for (int i = 0; i < 4; ++i) {
    sum += valueNoise(p * freq) * amp;
    freq *= 2.03;
    amp *= 0.5;
  }
  return sum;
}

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

float sampleLocalRippleTexture(int index, vec2 uv) {
  if (index == 0) return texture(u_LocalRippleTex0, uv).r;
  if (index == 1) return texture(u_LocalRippleTex1, uv).r;
  if (index == 2) return texture(u_LocalRippleTex2, uv).r;
  return 1.0;
}

float rippleHeight(vec2 uv) {
  if (!u_UseRippleMask) return 0.0;
  float a = texture(u_RippleMaskTex, uv).r;
  float b = texture(u_RippleMaskTex, uv * 1.9 + vec2(0.17, -0.11)).r;
  float c = texture(u_RippleMaskTex, uv * 3.6 + vec2(-0.23, 0.29)).r;
  float combined = a * 0.55 + b * 0.30 + c * 0.15;
  return smoothstep(0.18, 0.82, combined);
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

  vec2 rippleUv = v_WorldPos.xz * u_RippleTiling + vec2(u_Time * 0.028, -u_Time * 0.020);
  vec2 rippleUvFine = v_WorldPos.xz * (u_RippleTiling * 2.4) + vec2(-u_Time * 0.035, u_Time * 0.026);
  vec3 rippleN = normalize(mix(rippleNormal(rippleUv), rippleNormal(rippleUvFine), 0.48));
  N = normalize(mix(N, applyPlanarNormal(N, rippleN), saturate(u_RippleStrength)));

  vec3 localRippleAccum = vec3(0.0);
  float localRippleWeight = 0.0;
  float localRippleFoam = 0.0;
  for (int i = 0; i < MAX_LOCAL_RIPPLES; ++i) {
    if (i >= u_LocalRippleCount) break;
    vec2 delta = v_WorldPos.xz - u_LocalRipplePos[i];
    float radius = max(0.001, u_LocalRippleRadius[i]);
    vec2 dir = safeDir(u_LocalRippleDirection[i]);
    vec2 side = vec2(-dir.y, dir.x);
    float noiseScale = max(0.001, u_LocalRippleNoiseScale[i]);
    vec2 warp = vec2(
        fbm(delta * noiseScale + vec2(u_Time * u_LocalRippleNoiseSpeed[i], 1.7)),
        fbm(delta.yx * (noiseScale * 1.11) + vec2(-2.3, u_Time * u_LocalRippleNoiseSpeed[i] * 0.83))) -
        vec2(0.5);
    vec2 warpedDelta = delta + warp * (radius * 0.24 * saturate(u_LocalRippleNoiseStrength[i]));
    float lengthMeters = max(0.001, u_LocalRippleLength[i]);
    float widthMeters = max(0.001, u_LocalRippleWidth[i]);
    float along = dot(warpedDelta, dir);
    float across = dot(warpedDelta, side);
    float ellipse = sqrt((along * along) / (lengthMeters * lengthMeters) +
                         (across * across) / (widthMeters * widthMeters));
    if (ellipse > 1.0) continue;
    float dist = length(warpedDelta);

    float radiusMask = 1.0 - saturate(dist / radius);
    float ellipseMask = 1.0 - saturate(ellipse);
    float frontMask = 1.0 - smoothstep(0.10 * lengthMeters, 0.52 * lengthMeters, along);
    float trailMask = 1.0 - smoothstep(0.0, lengthMeters, -along);
    float wakeCore = exp(-abs(across) / max(0.18, widthMeters * 0.30)) * trailMask;
    float wakeBands = 0.5 + 0.5 * sin((-along * 0.58 + abs(across) * 1.32) * u_LocalRippleFrequency[i] -
                                      u_Time * u_LocalRippleSpeed[i] * 1.12);
    float sideChop = 0.5 + 0.5 * sin(across * (u_LocalRippleFrequency[i] * 1.55) + along * 0.30 -
                                     u_Time * u_LocalRippleSpeed[i] * 0.78);
    float disturbanceNoise = fbm(warpedDelta * (noiseScale * 1.7) + vec2(4.2, -3.1) +
                                 dir * (u_Time * u_LocalRippleNoiseSpeed[i] * 1.22));
    float falloff = pow(radiusMask * ellipseMask, max(0.1, u_LocalRippleFalloff[i]));
    vec2 localUv = warpedDelta * u_LocalRippleTiling[i] + dir * (u_Time * u_LocalRippleDriftSpeed[i]);
    float texMask = (u_LocalRippleUseTexture[i] != 0) ? sampleLocalRippleTexture(i, localUv) : 1.0;
    texMask = mix(0.45, 1.0, smoothstep(0.05, 0.85, texMask));
    float noise = fbm(warpedDelta * noiseScale + dir * (u_Time * u_LocalRippleNoiseSpeed[i]));
    float noiseMask = mix(1.0, mix(0.70, 1.0, smoothstep(0.20, 0.80, noise)), saturate(u_LocalRippleNoiseStrength[i]));
    float impactRing = 0.5 + 0.5 * sin(dist * (u_LocalRippleFrequency[i] * 0.82) - u_Time * u_LocalRippleSpeed[i] * 0.9);
    float objectDisturbance = frontMask * impactRing;
    float wakeDisturbance = wakeCore * mix(wakeBands, sideChop, 0.35);
    float chop = mix(0.82, 1.18, disturbanceNoise);
    float wave = mix(objectDisturbance, wakeDisturbance, 0.64) * chop;
    float mask = falloff * noiseMask * mix(1.0, texMask, (u_LocalRippleUseTexture[i] != 0) ? 1.0 : 0.0);
    float height = wave * mask * u_LocalRippleStrength[i] * max(0.0, u_LocalRippleMagnitude[i]);

    vec2 normalDir = normalize(mix((dist > 1e-4) ? (warpedDelta / dist) : dir, dir, 0.58));
    vec3 localN = normalize(vec3(-normalDir.x * height * 3.0,
                                 1.0 / max(0.22, 1.0 + u_LocalRippleMagnitude[i] + wakeCore * 0.8),
                                 -normalDir.y * height * 3.0));
    localRippleAccum += localN * (mask * (0.8 + wakeCore * 0.9));
    localRippleWeight += mask * (1.1 + wakeCore * 0.9);
    localRippleFoam += mask * max(wakeDisturbance, objectDisturbance * 0.65) * u_LocalRippleFoamBoost[i] * 2.25;
  }
  if (localRippleWeight > 1e-4) {
    vec3 localRippleN = normalize(localRippleAccum / localRippleWeight);
    N = normalize(mix(N, applyPlanarNormal(N, localRippleN), saturate(localRippleWeight * 1.4)));
  }

  float groundHeight = terrainHeightAt(v_Uv);
  float waterDepth = max(0.0, u_WaterLevel - groundHeight);
  float deepness = saturate(waterDepth / max(0.001, u_ShoreFadeDistance));
  float shallowMask = 1.0 - deepness;

  vec3 waterColor = mix(u_ShallowColor, u_BaseColor.rgb, deepness);

  float rippleBreakup = u_UseRippleMask ? max(rippleHeight(rippleUv), rippleHeight(rippleUvFine)) : 0.5;
  float shoreMask = 1.0 - smoothstep(0.0, max(0.001, u_ShoreFoamDepth), waterDepth);
  float foamEdge = 1.0 - smoothstep(0.0, max(0.001, u_ShoreFadeDistance * 0.65), waterDepth);
  vec2 foamDriftDir = safeDir(u_FoamDriftDirection);
  vec2 foamDrift = foamDriftDir * (u_Time * u_FoamDriftSpeed);
  float foamNoise = fbm(v_WorldPos.xz * max(0.001, u_FoamNoiseScale) + foamDrift + vec2(1.7, -2.3));
  float foamClumps = smoothstep(0.48, 0.82, foamNoise);
  float clumpMask = mix(1.0, foamClumps, saturate(u_FoamNoiseStrength));
  float foam = shoreMask * mix(0.45, 1.0, rippleBreakup) * u_ShoreFoamStrength;
  foam += foamEdge * mix(0.18, 0.55, rippleBreakup) * u_ShoreFoamStrength;
  foam *= mix(1.0, mix(0.72, 1.3, clumpMask), saturate(u_FoamNoiseStrength));
  foam += localRippleFoam;

  if (u_UseFoamNormal) {
    vec2 foamUv = v_WorldPos.xz * u_FoamTiling + foamDrift + vec2(-u_Time * 0.015, u_Time * 0.009);
    vec3 foamNormal = tangentNormal(u_FoamNormalTex, foamUv);
    float foamBreakup = saturate(length(foamNormal.xz));
    foam += shoreMask * foamBreakup * u_FoamStrength * 1.15;
    foam += foamEdge * foamBreakup * u_FoamStrength * 0.90;
    float driftingFoam = foamBreakup * clumpMask * u_FoamStrength * mix(0.0, 0.8, saturate(u_FoamNoiseStrength));
    float openWaterFoamMask = mix(shallowMask, 1.0 - deepness * 0.65, 0.35);
    foam += openWaterFoamMask * driftingFoam;
    N = normalize(mix(N, applyPlanarNormal(N, foamNormal), max(shoreMask * 0.25, foamEdge * 0.18)));
  }

  foam = smoothstep(0.18, 0.88, saturate(foam));

  vec3 lit = waterColor * vec3(0.06, 0.08, 0.10);
  float NdotV = saturate(dot(N, V));
  float fresnel = pow(1.0 - NdotV, 5.0);
  float gloss = 1.0 - saturate(u_Roughness);

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
    float specPower = mix(180.0, 22.0, saturate(u_Roughness));
    float spec = pow(saturate(dot(N, H)), specPower) * mix(0.08, 1.35, saturate(u_SpecularIntensity));
    vec3 lightCol = u_LightColor[i] * u_LightIntensity[i] * attenuation;
    lit += waterColor * lightCol * (NdotL * mix(0.12, 0.20, shallowMask));
    lit += lightCol * spec * mix(0.45, 1.15, fresnel);
  }

  float causticWave = 0.5 + 0.5 * sin((v_WorldPos.x + v_WorldPos.z) * 0.24 + u_Time * 2.7);
  float causticRipple = 0.5 + 0.5 * sin((v_WorldPos.x - v_WorldPos.z) * 0.41 - u_Time * 1.8);
  float caustics = shallowMask * rippleBreakup * causticWave * causticRipple * 0.18;
  lit += vec3(0.10, 0.20, 0.18) * caustics;

  vec3 skyReflect = mix(vec3(0.06, 0.15, 0.20), vec3(0.48, 0.64, 0.76), pow(1.0 - saturate(V.y * 0.5 + 0.5), 2.0));
  float reflectionMix = clamp(0.36 + fresnel * 0.70 + gloss * 0.14, 0.0, 0.96);
  vec3 finalColor = mix(lit, skyReflect, reflectionMix);
  finalColor = mix(finalColor, mix(u_FoamColor, vec3(1.0), 0.55), foam);

  float alpha = mix(0.24, u_WaterAlpha, deepness);
  alpha *= mix(1.0, 0.72, saturate(u_Clarity));
  alpha = max(alpha, foam * 0.45);

  float fog = fogFactorAt(u_CameraPos, v_WorldPos);
  finalColor = mix(finalColor, u_FogColor, fog);
  o_Color = vec4(finalColor, alpha);
}
