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

float saturate(float x) { return clamp(x, 0.0, 1.0); }
float softClip(float x, float threshold) { return x / (x + threshold); }

// ------------------------------------------------------------
// High-quality noise functions
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
    for (int i = 0; i < 6; ++i) {
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
// 3D Ripple System
// ------------------------------------------------------------
struct RippleData {
    float height;
    vec3 normal;
    float foam;
    float occlusion;
};

// Single ripple wave with dispersion (different frequencies travel at different speeds)
float rippleWave(vec2 pos, vec2 center, float time, float frequency, float speed, float falloff) {
    float dist = length(pos - center);
    float wavelength = 1.0 / max(0.001, frequency);
    float phase = dist * frequency * 6.28318 - time * speed;
    
    // Amplitude attenuation with distance (energy conservation)
    float amplitude = 1.0 / max(0.001, 1.0 + dist * falloff);
    
    // Dispersion: higher frequencies travel faster
    float dispersion = 1.0 + frequency * 0.3;
    phase *= dispersion;
    
    return sin(phase) * amplitude;
}

// Calculate 3D ripple field at a given world position
RippleData calculateRipples(vec2 worldXZ, vec2 ripplePos, float radius, 
                            vec2 lengthWidth, float strength, float magnitude,
                            float frequency, float speed, float falloffPower,
                            vec2 direction, float driftSpeed, float foamBoost,
                            float noiseScale, float noiseStrength, float noiseSpeed,
                            bool useTexture, sampler2D rippleTex, float tiling) {
    
    RippleData result;
    result.height = 0.0;
    result.normal = vec3(0.0, 1.0, 0.0);
    result.foam = 0.0;
    result.occlusion = 1.0;
    
    vec2 dir = length(direction) > 0.001 ? normalize(direction) : vec2(1.0, 0.0);
    vec2 side = vec2(-dir.y, dir.x);
    
    // Anisotropic distance for wake shape
    vec2 delta = worldXZ - ripplePos;
    float alongDist = dot(delta, dir);
    float acrossDist = dot(delta, side);
    
    // Elliptical falloff
    float ellipseDist = sqrt(
        (alongDist * alongDist) / (lengthWidth.x * lengthWidth.x) +
        (acrossDist * acrossDist) / (lengthWidth.y * lengthWidth.y)
    );
    
    float radialDist = length(delta);
    
    // Multi-layer ripple pattern
    float ripple1 = rippleWave(worldXZ, ripplePos, u_Time, frequency, speed, falloffPower * 0.5);
    float ripple2 = rippleWave(worldXZ, ripplePos, u_Time, frequency * 1.6, speed * 0.7, falloffPower * 0.7);
    float ripple3 = rippleWave(worldXZ, ripplePos, u_Time, frequency * 2.3, speed * 0.5, falloffPower * 0.9);
    
    // Combine ripple frequencies
    float ripplePattern = ripple1 * 0.5 + ripple2 * 0.3 + ripple3 * 0.2;
    
    // Add noise-based micro-variation
    float noiseTime = u_Time * noiseSpeed;
    float microNoise = fbm(delta * noiseScale + vec2(noiseTime, noiseTime * 0.7)) * noiseStrength;
    float macroNoise = warpedFbm(delta * noiseScale * 0.5, noiseTime) * noiseStrength * 0.5;
    
    ripplePattern += microNoise + macroNoise;
    
    // Distance-based falloff with noise modulation
    float falloff = 1.0 - saturate(ellipseDist);
    falloff = pow(falloff, falloffPower);
    falloff *= 1.0 - saturate(radialDist / max(0.001, radius));
    
    // Texture mask
    float texMask = 1.0;
    if (useTexture) {
        vec2 texUv = delta * tiling + dir * (u_Time * driftSpeed);
        texMask = texture(rippleTex, texUv).r;
        texMask = smoothstep(0.1, 0.9, texMask);
    }
    
    // Height displacement
    float heightDisplacement = ripplePattern * falloff * strength * magnitude * texMask;
    result.height = heightDisplacement;
    
    // Calculate normal from height field gradient
    float eps = 0.02;
    float hx = rippleWave(worldXZ + vec2(eps, 0.0), ripplePos, u_Time, frequency, speed, falloffPower * 0.5) * 0.5 +
               rippleWave(worldXZ + vec2(eps, 0.0), ripplePos, u_Time, frequency * 1.6, speed * 0.7, falloffPower * 0.7) * 0.3 +
               rippleWave(worldXZ + vec2(eps, 0.0), ripplePos, u_Time, frequency * 2.3, speed * 0.5, falloffPower * 0.9) * 0.2;
    
    float hy = rippleWave(worldXZ + vec2(0.0, eps), ripplePos, u_Time, frequency, speed, falloffPower * 0.5) * 0.5 +
               rippleWave(worldXZ + vec2(0.0, eps), ripplePos, u_Time, frequency * 1.6, speed * 0.7, falloffPower * 0.7) * 0.3 +
               rippleWave(worldXZ + vec2(0.0, eps), ripplePos, u_Time, frequency * 2.3, speed * 0.5, falloffPower * 0.9) * 0.2;
    
    float hc = ripple1 * 0.5 + ripple2 * 0.3 + ripple3 * 0.2;
    hc *= falloff * strength * magnitude * texMask;
    hx = hx * falloff * strength * magnitude * texMask;
    hy = hy * falloff * strength * magnitude * texMask;
    
    // Proper 3D normal from height field
    vec3 rippleNormal = normalize(vec3(
        -(hx - hc) / eps,
        1.0 / (1.0 + abs(heightDisplacement) * 5.0),
        -(hy - hc) / eps
    ));
    
    result.normal = rippleNormal;
    
    // Foam generation based on wave steepness
    float steepness = length(vec2(hx - hc, hy - hc)) / eps;
    float foamThreshold = 0.3 / (1.0 + magnitude * 0.5);
    result.foam = smoothstep(foamThreshold, foamThreshold * 1.5, steepness) * foamBoost * falloff;
    
    // Self-occlusion (ripple peaks shadow troughs)
    result.occlusion = 1.0 - abs(ripplePattern) * falloff * 0.3;
    
    return result;
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
        
        // Primary
        float phaseA = k * dot(worldXZ, dirA) - u_Time * u_WaveSpeed * (1.0 + fi * 0.3);
        height += primarySteepness * amp * cos(phaseA) * 0.5;
        gradient += primarySteepness * amp * k * sin(phaseA) * dirA * 0.5;
        
        // Secondary
        float phaseB = k * dot(worldXZ, dirB) - u_Time * u_SecondaryWaveSpeed * (1.0 + fi * 0.25);
        height += secondarySteepness * amp * cos(phaseB) * 0.5;
        gradient += secondarySteepness * amp * k * sin(phaseB) * dirB * 0.5;
    }
    
    float microDetail = fbm(worldXZ * 3.5 + vec2(u_Time * 0.08, u_Time * 0.06));
    float eps = 0.02;
    vec2 microGrad;
    microGrad.x = fbm((worldXZ + vec2(eps, 0.0)) * 3.5 + vec2(u_Time * 0.08, u_Time * 0.06));
    microGrad.y = fbm((worldXZ + vec2(0.0, eps)) * 3.5 + vec2(u_Time * 0.08, u_Time * 0.06));
    microGrad = (microGrad - microDetail) / eps;
    
    height += microDetail * 0.15 * u_WaveHeight;
    gradient += microGrad * 0.15 * u_WaveHeight;
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
    return alpha2 / (3.14159 * d * d);
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
void main() {
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    
    // Wave deformation
    float waveHeight;
    vec2 waveGrad;
    waveDeformation(v_WorldPos.xz, waveHeight, waveGrad);
    vec3 N = normalize(vec3(-waveGrad.x, 1.0, -waveGrad.y));
    
    // Accumulate 3D ripples
    RippleData totalRipples;
    totalRipples.height = 0.0;
    totalRipples.normal = vec3(0.0, 1.0, 0.0);
    totalRipples.foam = 0.0;
    totalRipples.occlusion = 1.0;
    
    float totalWeight = 0.0;
    
    for (int i = 0; i < MAX_LOCAL_RIPPLES; ++i) {
        if (i >= u_LocalRippleCount) break;
        
        sampler2D rippleTex;
        if (u_LocalRippleUseTexture[i] != 0) {
            if (i == 0) rippleTex = u_LocalRippleTex0;
            else if (i == 1) rippleTex = u_LocalRippleTex1;
            else rippleTex = u_LocalRippleTex2;
        }
        
        RippleData ripple = calculateRipples(
            v_WorldPos.xz,
            u_LocalRipplePos[i],
            u_LocalRippleRadius[i],
            vec2(u_LocalRippleLength[i], u_LocalRippleWidth[i]),
            u_LocalRippleStrength[i],
            u_LocalRippleMagnitude[i],
            u_LocalRippleFrequency[i],
            u_LocalRippleSpeed[i],
            u_LocalRippleFalloff[i],
            u_LocalRippleDirection[i],
            u_LocalRippleDriftSpeed[i],
            u_LocalRippleFoamBoost[i],
            u_LocalRippleNoiseScale[i],
            u_LocalRippleNoiseStrength[i],
            u_LocalRippleNoiseSpeed[i],
            u_LocalRippleUseTexture[i] != 0,
            rippleTex,
            u_LocalRippleTiling[i]
        );
        
        float weight = 1.0 - saturate(length(v_WorldPos.xz - u_LocalRipplePos[i]) / max(0.001, u_LocalRippleRadius[i]));
        
        totalRipples.height += ripple.height * weight;
        totalRipples.normal += ripple.normal * weight;
        totalRipples.foam += ripple.foam * weight;
        totalRipples.occlusion = min(totalRipples.occlusion, ripple.occlusion);
        totalWeight += weight;
    }
    
    if (totalWeight > 0.001) {
        totalRipples.normal = normalize(totalRipples.normal / totalWeight);
        
        // Blend ripple normal with base normal
        float rippleBlend = saturate(totalWeight * 1.5);
        N = normalize(mix(N, totalRipples.normal, rippleBlend));
    }
    
    // Procedural micro-ripples
    vec2 rippleDriftA = safeDir(u_WaveDirection) * (u_Time * 0.15 * max(0.05, u_WaveSpeed));
    vec2 rippleDriftB = -safeDir(u_SecondaryWaveDirection) * (u_Time * 0.22 * max(0.05, u_SecondaryWaveSpeed));
    
    vec3 rippleN1 = proceduralRippleNormal(v_WorldPos.xz, u_RippleTiling, rippleDriftA);
    vec3 rippleN2 = proceduralRippleNormal(v_WorldPos.xz, u_RippleTiling * 2.4, rippleDriftB);
    vec3 rippleN = normalize(mix(rippleN1, rippleN2, 0.5));
    
    vec3 T = vec3(1.0, 0.0, 0.0);
    vec3 B = vec3(0.0, 0.0, 1.0);
    vec3 tangentN = normalize(T * rippleN.x + N * rippleN.y + B * rippleN.z);
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
    
    // Add ripple-generated foam
    foam += totalRipples.foam * 2.0;
    
    float streakNoise = gradientNoise(v_WorldPos.xz * 0.8 + foamDrift * 0.5);
    float streaks = smoothstep(0.62, 0.82, streakNoise) * (1.0 - deepness * 0.7);
    foam += streaks * u_FoamStrength * 0.4 * rippleBreakup;
    
    float foamNormalStrength = max(shoreMask * 0.3, foamEdge * 0.2);
    vec3 foamPerturb = proceduralRippleNormal(v_WorldPos.xz, u_FoamTiling, foamDrift);
    N = normalize(mix(N, tangentN * 0.5 + foamPerturb * 0.5, foamNormalStrength));
    
    foam = smoothstep(0.18, 0.88, saturate(foam));
    
    // ------------------------------------------------------------
    // Lighting with self-shadowing
    // ------------------------------------------------------------
    vec3 ambient = waterColor * 0.04 * totalRipples.occlusion;
    vec3 lit = ambient;
    
    float NdotV = saturate(dot(N, V));
    float fresnel = schlickFresnel(NdotV, WATER_F0);
    
    float metallicInfluence = saturate(u_Metallic);
    float specTint = mix(1.0, 0.65, metallicInfluence);
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
        
        // Self-shadowing from ripples
        float shadow = 1.0;
        if (NdotL > 0.0) {
            // Simulate self-shadowing based on ripple height
            float rippleShadow = totalRipples.height * 2.0;
            shadow = 1.0 - saturate(rippleShadow * (1.0 - NdotL)) * 0.5;
            shadow *= totalRipples.occlusion;
        }
        
        NdotL *= shadow;
        if (NdotL <= 0.0) continue;
        
        vec3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        vec3 lightCol = u_LightColor[i] * u_LightIntensity[i] * attenuation;
        
        // Two-lobe specular
        float specSoft = ggxSpecular(NdotH, roughness);
        float specSparkle = ggxSpecular(NdotH, roughness * 0.22);
        specSparkle = softClip(specSparkle, 2.5);
        
        float sparkleBoost = 0.3 + 0.7 * fresnel;
        float spec = (specSoft * 0.4 + specSparkle * 3.0 * sparkleBoost) * u_SpecularIntensity;
        
        // Ripple sparkle enhancement
        spec *= 1.0 + totalRipples.foam * 2.0;
        
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
        
        // Subsurface scattering approximation for ripple peaks
        float sss = 0.0;
        if (totalRipples.height > 0.0) {
            vec3 backLight = -L;
            float sssFactor = saturate(dot(V, -backLight)) * saturate(totalRipples.height * 0.5);
            sss = sssFactor * 0.15 * shadow;
        }
        
        lit += waterColor * lightCol * (NdotL * mix(0.10, 0.18, shallowMask) + sss);
        
        vec3 specColor = lightCol * spec * specTint;
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
    caustics *= shallowMask * 0.22 * totalRipples.occlusion;
    
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