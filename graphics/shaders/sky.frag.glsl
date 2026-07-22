// Procedural sky + clouds (demo)
#version 410 core

#include "fog.glsl"

in vec2 v_Uv;
out vec4 o_Color;

uniform vec3 u_CameraPos;
uniform vec3 u_CamForward;
uniform vec3 u_CamRight;
uniform vec3 u_CamUp;
uniform float u_Aspect;
uniform float u_TanHalfFov;
uniform float u_Time;

uniform vec3 u_HorizonColor;
uniform vec3 u_ZenithColor;

uniform int u_SunEnabled;
uniform vec3 u_SunDir;
uniform vec3 u_SunTint;
uniform float u_SunDiscIntensity;
uniform float u_SunDiscSize;

uniform int u_SkyType;  // 0=Day,1=Sunset,2=Night,3=Overcast,4=Storm

uniform int u_CloudsEnabled;
uniform int u_CloudType;     // 0=None,1=Wispy,2=Scattered,3=Broken,4=Overcast,5=Storm
uniform int u_CloudQuality;  // 0=Low,1=Medium,2=High,3=Ultra
uniform float u_CloudCoverage;
uniform float u_CloudDensity;
uniform float u_CloudSpeed;
uniform vec2 u_CloudWindDir;
uniform float u_CloudTimeScale;
uniform float u_CloudTurbulence;
uniform float u_CloudScale;
uniform float u_CloudAbsorption;
uniform float u_CloudHeightMeters;

uniform int u_StarsEnabled;
uniform float u_StarsIntensity;
uniform float u_StarsDensity;
uniform float u_StarsSize;
uniform float u_StarsTwinkleStrength;
uniform float u_StarsTwinkleSpeed;
uniform uint u_StarsSeed;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float hash12(vec2 p) {
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

float fbm(vec2 p, int octaves) {
  float sum = 0.0;
  float amp = 0.5;
  float freq = 1.0;
  for (int i = 0; i < 8; ++i) {
    if (i >= octaves) break;
    sum += amp * valueNoise(p * freq);
    freq *= 2.02;
    amp *= 0.5;
  }
  return sum;
}

// Equirectangular mapping for a direction on the unit sphere.
vec2 dirToUv(vec3 d) {
  vec3 n = normalize(d);
  float u = atan(n.z, n.x) * (0.15915494309) + 0.5;  // 1/(2*pi)
  float v = asin(clamp(n.y, -1.0, 1.0)) * (0.31830988618) + 0.5; // 1/pi
  return vec2(u, v);
}

vec3 starsColor(vec3 ray) {
  // Only show stars in the upper hemisphere.
  float upMask = smoothstep(0.02, 0.20, ray.y);
  if (upMask <= 0.0) return vec3(0.0);

  vec2 suv = dirToUv(ray);
  // Scale controls star field granularity; higher -> more cells.
  float grid = mix(420.0, 980.0, saturate(u_StarsDensity));
  vec2 p = suv * grid;
  vec2 cell = floor(p);
  vec2 f = fract(p) - 0.5;

  // Stable random per-cell with seed.
  float seedF = float(u_StarsSeed & 1023u) * 0.001953125; // /512
  float r0 = hash12(cell + vec2(seedF, seedF * 17.0));
  float r1 = hash12(cell + vec2(19.7 + seedF * 3.0, 4.1 + seedF * 11.0));

  // Star existence threshold (density). Higher density => more stars.
  float density = mix(0.994, 0.955, saturate(u_StarsDensity));
  float exists = step(density, r0);
  if (exists <= 0.0) return vec3(0.0);

  // Star position within cell.
  vec2 ofs = vec2(r0, r1) - 0.5;
  float d = length(f - ofs);

  // Tiny crisp specs: keep sizes very small, with a sharp falloff.
  float size = mix(0.00075, 0.0026, saturate(u_StarsSize));
  float x = saturate(1.0 - (d / max(1e-6, size)));
  float star = pow(x, 10.0);

  // Occasional brighter "core" for variety (still a spec).
  float core = pow(x, 22.0) * (0.6 + 0.8 * r1);
  star = max(star, core);
  // Slightly vary brightness and color temperature.
  float bright = mix(0.7, 3.2, r1 * r1);
  vec3 cold = vec3(0.75, 0.85, 1.0);
  vec3 warm = vec3(1.0, 0.88, 0.72);
  vec3 tint = mix(cold, warm, r0);

  // Twinkle (very subtle).
  float tw = 1.0;
  float twS = saturate(u_StarsTwinkleStrength);
  if (twS > 0.0) {
    float phase = (r0 * 6.28318) + u_Time * (u_StarsTwinkleSpeed * 1.7 + r1);
    tw = 1.0 + (sin(phase) * 0.5 + 0.5) * twS * 0.45;
  }

  return (tint * star * bright * tw * u_StarsIntensity) * upMask;
}

vec3 skyGradient(vec3 ray) {
  float t = saturate(ray.y * 0.5 + 0.5);
  vec3 col = mix(u_HorizonColor, u_ZenithColor, pow(t, 0.9));

  if (u_SkyType == 1) {  // sunset
    vec3 sunset = vec3(1.0, 0.46, 0.18);
    col = mix(col, sunset, pow(saturate(1.0 - t), 2.0) * 0.35);
  } else if (u_SkyType == 2) {  // night
    col *= 0.12;
    col += vec3(0.01, 0.02, 0.04) * (1.0 - t);
  } else if (u_SkyType == 3) {  // overcast
    col = mix(col, vec3(0.55, 0.58, 0.62), 0.65);
  } else if (u_SkyType == 4) {  // storm
    col = mix(col, vec3(0.20, 0.22, 0.26), 0.75);
  }

  return col;
}

float sunDisc(vec3 ray) {
  vec3 sd = normalize(u_SunDir);
  float d = saturate(dot(ray, sd));
  float size = max(0.0001, u_SunDiscSize);
  float k = pow(d, 2000.0 / size);
  return k * u_SunDiscIntensity;
}

void main() {
  vec2 ndc = v_Uv * 2.0 - 1.0;
  ndc.x *= u_Aspect;
  vec3 ray = normalize(u_CamForward + ndc.x * u_CamRight * u_TanHalfFov + ndc.y * u_CamUp * u_TanHalfFov);

  vec3 col = skyGradient(ray);

  // Stars (mostly for night, but controllable).
  if (u_StarsEnabled != 0) {
    float nightMask = (u_SkyType == 2) ? 1.0 : 0.35;
    col += starsColor(ray) * nightMask;
  }

  if (u_SunEnabled != 0) {
    float s = sunDisc(ray);
    col += u_SunTint * s;
  }

  if (u_CloudsEnabled != 0 && u_CloudType != 0) {
    // Dome-mapped clouds (avoids the "flat ceiling" look).
    // Uses ray direction to map onto a sky dome; translation has minimal parallax (more realistic for distant clouds).
    float horizon = saturate((ray.y + 0.08) * 6.5);  // fade near horizon so it blends into atmospheric haze

    // Project direction onto a dome-friendly 2D domain (more detail near zenith, stretched toward horizon).
    float denom = max(0.18, ray.y + 0.35);
    vec2 p = (ray.xz / denom) * (0.55 * u_CloudScale);
    vec2 wind = normalize(u_CloudWindDir);
    if (wind.x != wind.x) wind = vec2(1.0, 0.0); // NaN guard
    p += wind * (u_Time * u_CloudSpeed * u_CloudTimeScale);
    p += vec2(0.0, 1.0) * (u_Time * u_CloudSpeed * u_CloudTimeScale * 0.35);

    int oct = 5;
    if (u_CloudQuality == 0) oct = 3;
    if (u_CloudQuality == 1) oct = 5;
    if (u_CloudQuality == 2) oct = 6;
    if (u_CloudQuality >= 3) oct = 7;

    // Cloud type presets.
    float cov = clamp(u_CloudCoverage, 0.0, 1.0);
    float den = clamp(u_CloudDensity, 0.0, 1.0);
    if (u_CloudType == 1) { cov *= 0.55; den *= 0.55; }
    if (u_CloudType == 2) { cov *= 0.80; den *= 0.75; }
    if (u_CloudType == 3) { cov *= 1.05; den *= 0.95; }
    if (u_CloudType == 4) { cov = max(cov, 0.75); den = max(den, 0.95); }
    if (u_CloudType == 5) { cov = max(cov, 0.82); den = max(den, 1.00); }

    // Domain warp to break symmetry.
    float turb = clamp(u_CloudTurbulence, 0.0, 1.0);
    vec2 warp = vec2(fbm(p * 0.65, 3), fbm(p * 0.72 + 19.7, 3)) - 0.5;
    vec2 q = p + warp * mix(0.35, 1.10, turb);

    float n = fbm(q, oct);
    float detail = fbm(q * 2.4 + 11.3, 3);
    n = mix(n, n * 0.8 + detail * 0.2, mix(0.25, 0.60, turb));

    // Coverage shifts the threshold; density controls thickness.
    float threshold = mix(0.72, 0.40, cov);
    float edge = smoothstep(threshold, threshold + 0.20, n);
    float cloud = edge * den * horizon;

    // Cheap lighting: brighten toward sun, darken with absorption (also fades slightly toward horizon).
    float sunDot = saturate(dot(ray, normalize(u_SunDir)));
    float silver = pow(sunDot, 10.0) * 0.65;
    float shade = 1.0 - cloud * u_CloudAbsorption;

    vec3 cloudCol = vec3(1.0) * (0.72 + 0.28 * sunDot);
    cloudCol += u_SunTint * silver;
    cloudCol *= shade;
    cloudCol = mix(cloudCol, col, saturate((1.0 - horizon) * 0.75));

    // Night: darker, bluer clouds (moonlight feel).
    if (u_SkyType == 2) {
      cloudCol *= vec3(0.35, 0.42, 0.55);
      cloudCol += u_SunTint * (0.06 + 0.06 * sunDot);
    }

    col = mix(col, cloudCol, saturate(cloud));
  }

  // Horizon fog blend (cheap but helps depth/edge hiding).
  if (u_FogEnabled != 0) {
    // Probe fog along the view ray at a far distance so the sky picks up haze near the horizon.
    float probeDist = max(60.0, u_FogEnd);
    vec3 probePos = u_CameraPos + ray * probeDist;
    float fogF = fogFactorAt(u_CameraPos, probePos);

    // Emphasize near-horizon.
    float horizon = saturate((0.28 - ray.y) * 3.2);
    fogF *= horizon;

    // Slightly brighten the fog against the sky so it's visible.
    vec3 fogCol = u_FogColor * 1.35;
    col = mix(col, fogCol, saturate(fogF));
  }

  o_Color = vec4(col, 1.0);
}
