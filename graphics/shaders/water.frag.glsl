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

const float WATER_F0 = 0.02;
const float WATER_ABSORPTION = 0.28;
const float PI = 3.14159265;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
float softClip(float x, float threshold) { return x / (x + threshold); }

// ------------------------------------------------------------
// Noise functions
// ------------------------------------------------------------
float hash2(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

float gradientNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash2(i + vec2(0.0, 0.0)), hash2(i + vec2(1.0, 0.0)), u.x),
        mix(hash2(i + vec2(0.0, 1.0)), hash2(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

float fbm(vec2 p) {
    float sum = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    for (int i = 0; i < 5; ++i) {
        sum += gradientNoise(p * freq) * amp;
        freq *= 2.17;
        amp *= 0.5;
    }
    return sum;
}

float warpedFbm(vec2 p, float time) {
    float warp1 = fbm(p * 1.3 + vec2(time * 0.15, time * 0.12));
    float warp2 = fbm(p * 1.7 + vec2(time * -0.1, time * 0.18) + warp1 * 2.0);
    return fbm(p * 2.1 + vec2(warp1 * 1.8, warp2 * 1.5));
}

// ------------------------------------------------------------
// Wave system
// ------------------------------------------------------------
vec2 safeDir(vec2 v) {
    float lenSq = dot(v, v);
    if (lenSq < 1e-5) return vec2(1.0, 0.0);
    return v * inversesqrt(lenSq);
}

void waveDeformation(vec2 worldXZ, out float height, out vec2 gradient) {
    vec2 dirA = safeDir(u_WaveDirection);
    vec2 dirB = safeDir(u_SecondaryWaveDirection);
    
    height = 0.0;
    gradient = vec2(0.0);
    
    float primarySteepness = u_WaveHeight * 0.6;
    float secondarySteepness = u_SecondaryWaveHeight * 0.55;
    
    for (int i = 0; i < 3; i++) {
        float fi = float(i);
        float freq = 1.0 + fi * 0.7;
        float amp = 1.0 / (1.0 + fi * 0.8);
        float wavelength = u_WaveScale / freq;
        float k = 6.28318 / max(0.001, wavelength);
        
        float phaseA = k * dot(worldXZ, dirA) - u_Time * u_WaveSpeed * (1.0 + fi * 0.3);
        height += primarySteepness * amp * cos(phaseA) * 0.5;
        gradient += primarySteepness * amp * k * sin(phaseA) * dirA * 0.5;
        
        float phaseB = k * dot(worldXZ, dirB) - u_Time * u_SecondaryWaveSpeed * (1.0 + fi * 0.25);
        height += secondarySteepness * amp * cos(phaseB) * 0.5;
        gradient += secondarySteepness * amp * k * sin(phaseB) * dirB * 0.5;
    }
}

vec3 proceduralRippleNormal(vec2 worldXZ, float tiling, vec2 drift) {
    float scale = 1.0 / max(0.001, tiling);
    float eps = 0.015;
    float h = fbm(worldXZ * scale + drift);
    float hx = fbm((worldXZ + vec2(eps, 0.0)) * scale + drift);
    float hy = fbm((worldXZ + vec2(0.0, eps)) * scale + drift);
    vec2 grad = vec2(hx - h, hy - h) / eps;
    return normalize(vec3(-grad.x * 1.8, 1.0, -grad.y * 1.8));
}

float terrainHeightAt(vec2 uv) {
    if (!u_UseHeightMap || u_ShoreTerrainHeightScale <= 0.0) return u_ShoreTerrainBaseY;
    float h = texture(u_HeightMapTex, uv * u_MapUvTiling).r;
    if (u_HeightMapInvert != 0) h = 1.0 - h;
    return u_ShoreTerrainBaseY + h * u_ShoreTerrainHeightScale * u_HeightMapStrength;
}

float schlickFresnel(float NdotV, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
}

float ggxSpecular(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float d = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * d * d);
}

// ------------------------------------------------------------
// Local ripple normal perturbation (subtle, preserves water look)
// ------------------------------------------------------------
vec3 calculateLocalRippleNormal(vec2 worldXZ, int rippleIndex) {
    vec2 center = u_LocalRipplePos[rippleIndex];
    vec2 dir = length(u_LocalRippleDirection[rippleIndex]) > 0.001 
        ? normalize(u_LocalRippleDirection[rippleIndex]) 
        : vec2(1.0, 0.0);
    vec2 side = vec2(-dir.y, dir.x);
    
    vec2 delta = worldXZ - center;
    float alongDist = dot(delta, dir);
    float acrossDist = dot(delta, side);
    
    // Elliptical distance for wake shape
    float ellipseDist = sqrt(
        (alongDist * alongDist) / max(0.001, u_LocalRippleLength[rippleIndex] * u_LocalRippleLength[rippleIndex]) +
        (acrossDist * acrossDist) / max(0.001, u_LocalRippleWidth[rippleIndex] * u_LocalRippleWidth[rippleIndex])
    );
    
    float radialDist = length(delta);
    float radius = max(0.001, u_LocalRippleRadius[rippleIndex]);
    
    // Smooth falloff from center
    float falloff = 1.0 - saturate(ellipseDist);
    falloff = pow(falloff, u_LocalRippleFalloff[rippleIndex]);
    falloff *= 1.0 - saturate(radialDist / radius);
    
    if (falloff < 0.001) return vec3(0.0, 1.0, 0.0);
    
    // Multi-frequency ripple waves
    float freq = u_LocalRippleFrequency[rippleIndex];
    float speed = u_LocalRippleSpeed[rippleIndex];
    float mag = u_LocalRippleMagnitude[rippleIndex] * u_LocalRippleStrength[rippleIndex];
    
    // Calculate ripple height at this point
    float phase = radialDist * freq * 6.28318 - u_Time * speed;
    float rippleHeight = sin(phase) * falloff * mag;
    
    // Add subtle noise variation
    float noiseTime = u_Time * u_LocalRippleNoiseSpeed[rippleIndex];
    float noise = fbm(delta * u_LocalRippleNoiseScale[rippleIndex] + noiseTime) * u_LocalRippleNoiseStrength[rippleIndex];
    rippleHeight *= 1.0 + noise * 0.5;
    
    // Calculate normal from height gradient
    float eps = 0.03;
    float hx = sin((length(worldXZ + vec2(eps, 0.0) - center)) * freq * 6.28318 - u_Time * speed) * falloff * mag;
    float hy = sin((length(worldXZ + vec2(0.0, eps) - center)) * freq * 6.28318 - u_Time * speed) * falloff * mag;
    
    // Gentle normal perturbation - ripples modulate the existing surface, not replace it
    float normalStrength = abs(rippleHeight) * 3.0;
    vec3 rippleN = normalize(vec3(
        -(hx - rippleHeight) / eps * normalStrength,
        1.0,
        -(hy - rippleHeight) / eps * normalStrength
    ));
    
    // Blend toward flat surface as ripples fade
    return mix(vec3(0.0, 1.0, 0.0), rippleN, falloff);
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
void main() {
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    
    // Base wave deformation
    float waveHeight;
    vec2 waveGrad;
    waveDeformation(v_WorldPos.xz, waveHeight, waveGrad);
    vec3 N = normalize(vec3(-waveGrad.x, 1.0, -waveGrad.y));
    
    // Accumulate local ripple normals (additive perturbation)
    vec3 rippleNormalSum = vec3(0.0);
    float rippleWeightSum = 0.0;
    float totalRippleFoam = 0.0;
    
    for (int i = 0; i < MAX_LOCAL_RIPPLES; ++i) {
        if (i >= u_LocalRippleCount) break;
        
        vec3 localN = calculateLocalRippleNormal(v_WorldPos.xz, i);
        vec2 center = u_LocalRipplePos[i];
        float dist = length(v_WorldPos.xz - center);
        float radius = max(0.001, u_LocalRippleRadius[i]);
        float weight = 1.0 - saturate(dist / radius);
        
        if (weight > 0.001) {
            // Convert from tangent space to world space
            vec3 T = normalize(cross(N, vec3(0.0, 0.0, 1.0)));
            if (length(T) < 0.01) T = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
            vec3 B = normalize(cross(N, T));
            
            vec3 worldRippleN = normalize(
                T * localN.x + N * localN.y + B * localN.z
            );
            
            rippleNormalSum += worldRippleN * weight;
            rippleWeightSum += weight;
            
            // Subtle foam at ripple crests
            float foamBoost = u_LocalRippleFoamBoost[i];
            if (foamBoost > 0.0) {
                float ripplePhase = dist * u_LocalRippleFrequency[i] * 6.28318 - u_Time * u_LocalRippleSpeed[i];
                float crest = saturate(sin(ripplePhase));
                totalRippleFoam += crest * weight * foamBoost * 3.0;
            }
        }
    }
    
    // Blend ripple normals with base water normal (ripples are subtle)
    if (rippleWeightSum > 0.001) {
        vec3 blendedRippleN = normalize(rippleNormalSum / rippleWeightSum);
        // Ripples perturb the surface but don't replace it
        float rippleBlend = saturate(rippleWeightSum * 0.6);
        N = normalize(mix(N, blendedRippleN, rippleBlend));
    }
    
    // Procedural micro-ripples (wind ripples)
    vec2 rippleDriftA = safeDir(u_WaveDirection) * (u_Time * 0.15 * max(0.05, u_WaveSpeed));
    vec2 rippleDriftB = -safeDir(u_SecondaryWaveDirection) * (u_Time * 0.22 * max(0.05, u_SecondaryWaveSpeed));
    
    vec3 rippleN1 = proceduralRippleNormal(v_WorldPos.xz, u_RippleTiling, rippleDriftA);
    vec3 rippleN2 = proceduralRippleNormal(v_WorldPos.xz, u_RippleTiling * 2.4, rippleDriftB);
    vec3 microRippleN = normalize(mix(rippleN1, rippleN2, 0.5));
    
    vec3 T = normalize(cross(N, vec3(0.0, 0.0, 1.0)));
    if (length(T) < 0.01) T = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    vec3 B = normalize(cross(N, T));
    vec3 tangentN = normalize(T * microRippleN.x + N * microRippleN.y + B * microRippleN.z);
    N = normalize(mix(N, tangentN, saturate(u_RippleStrength * 1.2)));
    
    // Depth and shore
    float groundHeight = terrainHeightAt(v_Uv);
    float waterDepth = max(0.0, u_WaterLevel - groundHeight);
    float deepness = saturate(waterDepth / max(0.001, u_ShoreFadeDistance));
    float shallowMask = 1.0 - deepness;
    
    float absorption = exp(-WATER_ABSORPTION * waterDepth);
    vec3 waterColor = mix(u_ShallowColor, u_BaseColor.rgb, 1.0 - absorption);
    
    // Foam
    vec2 foamDrift = safeDir(u_FoamDriftDirection) * (u_Time * u_FoamDriftSpeed);
    
    float foamNoise1 = gradientNoise(v_WorldPos.xz * u_FoamTiling + foamDrift);
    float foamNoise2 = gradientNoise(v_WorldPos.xz * u_FoamTiling * 2.5 - foamDrift * 0.7);
    float foamNoise3 = fbm(v_WorldPos.xz * u_FoamNoiseScale * 4.0 + foamDrift * 1.5);
    
    float foamPattern = foamNoise1 * 0.5 + foamNoise2 * 0.3 + foamNoise3 * 0.2;
    float foamClumps = smoothstep(0.45, 0.78, foamPattern);
    float clumpMask = mix(1.0, foamClumps, saturate(u_FoamNoiseStrength));
    
    float breakupNoise = warpedFbm(v_WorldPos.xz * 3.0, u_Time * 0.2);
    float rippleBreakup = smoothstep(0.3, 0.7, breakupNoise);
    
    float shoreMask = 1.0 - smoothstep(0.0, max(0.001, u_ShoreFoamDepth), waterDepth);
    float foamEdge = 1.0 - smoothstep(0.0, max(0.001, u_ShoreFadeDistance * 0.65), waterDepth);
    
    float foam = 0.0;
    foam += shoreMask * mix(0.45, 1.0, rippleBreakup) * u_ShoreFoamStrength;
    foam += foamEdge * mix(0.18, 0.55, rippleBreakup) * u_ShoreFoamStrength;
    foam *= mix(1.0, mix(0.72, 1.3, clumpMask), saturate(u_FoamNoiseStrength));
    
    // Add subtle ripple foam
    foam += totalRippleFoam * 0.5;
    
    float streakNoise = gradientNoise(v_WorldPos.xz * 0.8 + foamDrift * 0.5);
    float streaks = smoothstep(0.62, 0.82, streakNoise) * (1.0 - deepness * 0.7);
    foam += streaks * u_FoamStrength * 0.4 * rippleBreakup;
    
    foam = smoothstep(0.18, 0.88, saturate(foam));
    
    // ------------------------------------------------------------
    // Lighting
    // ------------------------------------------------------------
    vec3 ambient = waterColor * 0.04;
    vec3 lit = ambient;
    
    float NdotV = saturate(dot(N, V));
    float fresnel = schlickFresnel(NdotV, WATER_F0);
    
    float metallicInfluence = saturate(u_Metallic);
    float roughness = saturate(u_Roughness);
    
    for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; ++i) {
        vec3 L;
        float attenuation = 1.0;
        
        if (u_LightType[i] == 0) {
            L = normalize(-u_LightDir[i]);
        } else {
            vec3 toLight = u_LightPos[i] - v_WorldPos;
            float dist = length(toLight);
            L = (dist > 1e-4) ? toLight / dist : vec3(0.0, 1.0, 0.0);
            float range = max(0.001, u_LightRange[i]);
            float falloff = saturate(1.0 - dist / range);
            attenuation = falloff * falloff;
        }
        
        float NdotL = saturate(dot(N, L));
        if (NdotL <= 0.0) continue;
        
        vec3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        vec3 lightCol = u_LightColor[i] * u_LightIntensity[i] * attenuation;
        
        // Two-lobe specular for realistic water sparkle
        float specSoft = ggxSpecular(NdotH, roughness);
        float specSparkle = ggxSpecular(NdotH, roughness * 0.22);
        specSparkle = softClip(specSparkle, 2.5);
        
        float sparkleBoost = 0.3 + 0.7 * fresnel;
        float spec = (specSoft * 0.4 + specSparkle * 3.0 * sparkleBoost) * u_SpecularIntensity;
        
        // Ripple areas get extra sparkle from perturbed normals
        spec *= 1.0 + rippleWeightSum * 0.5;
        
        // Sun glitter path for directional lights
        if (u_LightType[i] == 0) {
            vec3 lightDir = normalize(-u_LightDir[i]);
            vec3 sunReflect = reflect(-lightDir, N);
            float sunGlitter = saturate(dot(sunReflect, V));
            
            float glitter1 = pow(sunGlitter, mix(600.0, 2000.0, roughness));
            float glitter2 = pow(sunGlitter, mix(2000.0, 8000.0, roughness * 0.5));
            
            glitter1 = softClip(glitter1, 0.2);
            glitter2 = softClip(glitter2, 0.08);
            
            float sunStreak = (glitter1 * 0.6 + glitter2 * 0.4);
            spec += sunStreak * fresnel * u_SpecularIntensity * 1.8;
        }
        
        // Diffuse
        lit += waterColor * lightCol * (NdotL * mix(0.10, 0.18, shallowMask));
        
        // Specular
        vec3 specColor = lightCol * spec;
        specColor = mix(specColor, specColor * waterColor, metallicInfluence * 0.6);
        lit += specColor;
    }
    
    // Caustics
    vec2 causticCoord1 = v_WorldPos.xz * 0.6 + vec2(u_Time * 0.35, u_Time * 0.28);
    vec2 causticCoord2 = v_WorldPos.xz * 0.9 - vec2(u_Time * 0.22, -u_Time * 0.32);
    
    float caustic1 = warpedFbm(causticCoord1, u_Time * 0.5);
    float caustic2 = fbm(causticCoord2 + vec2(sin(u_Time * 0.4), cos(u_Time * 0.35)));
    
    float caustics = caustic1 * 0.6 + caustic2 * 0.4;
    caustics = smoothstep(0.38, 0.72, caustics);
    caustics *= shallowMask * 0.22;
    
    lit += vec3(0.12, 0.24, 0.20) * caustics;
    
    float microCaustic = fbm(v_WorldPos.xz * 4.5 + vec2(u_Time * 1.2, -u_Time * 0.9));
    microCaustic = smoothstep(0.55, 0.78, microCaustic) * shallowMask * 0.08;
    lit += vec3(0.08, 0.16, 0.14) * microCaustic;
    
    // Reflection
    vec3 R = reflect(-V, N);
    float skyGrad = pow(1.0 - saturate(R.y * 0.5 + 0.5), 2.0);
    
    vec3 skyHorizon = vec3(0.06, 0.15, 0.22);
    vec3 skyZenith = vec3(0.48, 0.64, 0.76);
    vec3 skyReflect = mix(skyHorizon, skyZenith, skyGrad);
    
    float cloudNoise = fbm(R.xz * 0.8 + vec2(u_Time * 0.02, u_Time * 0.015)) * 0.15;
    skyReflect = mix(skyReflect, skyReflect * 1.1 + vec3(0.05), cloudNoise);
    
    float gloss = 1.0 - roughness;
    float reflectionMix = clamp(fresnel * 0.82 + gloss * 0.10, 0.0, 0.96);
    
    vec3 reflectionColor = mix(vec3(1.0), waterColor * 0.7, metallicInfluence * 0.3);
    
    vec3 finalColor = mix(lit, skyReflect * reflectionColor, reflectionMix);
    finalColor = mix(finalColor, mix(u_FoamColor, vec3(1.0), 0.45), foam);
    
    // Alpha
    float alpha = mix(0.28, u_WaterAlpha, deepness);
    alpha *= mix(1.0, 0.68, saturate(u_Clarity));
    alpha = max(alpha, foam * 0.5);
    
    float edgeFade = smoothstep(0.0, 0.3, waterDepth);
    alpha *= mix(0.5, 1.0, edgeFade);
    
    float fog = fogFactorAt(u_CameraPos, v_WorldPos);
    finalColor = mix(finalColor, u_FogColor, fog);
    
    o_Color = vec4(finalColor, alpha);
}