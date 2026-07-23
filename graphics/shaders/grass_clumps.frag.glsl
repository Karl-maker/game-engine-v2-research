// Grass carpet fragment shader (single-layer, shader-driven density + camera haze)

#version 410 core

in vec2 v_Uv;
in vec3 v_WorldPos;
in float v_Var;
in float v_Fade;
in float v_ViewDist;

out vec4 o_Color;

uniform sampler2D u_AlbedoTex;
uniform int u_UseAlbedo;
uniform float u_AlbedoUvScale;

uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

uniform vec3 u_SpeciesTint;
uniform float u_CarpetHaze;
uniform float u_StylizedBands;
uniform float u_CarpetThickness;
uniform float u_Time;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float hash21(vec2 p) {
  p = fract(p * vec2(127.1, 311.7));
  p += dot(p, p + 41.19);
  return fract(p.x * p.y);
}

float valueNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float a = hash21(i);
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));
  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
  float sum = 0.0;
  float amp = 0.5;
  for (int i = 0; i < 4; ++i) {
    sum += valueNoise(p) * amp;
    p = p * 2.03 + vec2(13.7, 8.2);
    amp *= 0.5;
  }
  return sum;
}

vec3 softTexture(vec2 uv) {
  vec3 c = texture(u_AlbedoTex, uv).rgb * 0.50;
  c += texture(u_AlbedoTex, uv + vec2(0.023, 0.011)).rgb * 0.26;
  c += texture(u_AlbedoTex, uv + vec2(-0.019, 0.024)).rgb * 0.24;
  return c;
}

void main() {
  float haze = saturate(u_CarpetHaze);
  float thick = saturate(u_CarpetThickness);

  // Short card silhouette, then carved into many broken grass fibers.
  float height = saturate(v_Uv.y);
  float windT = u_Time * 0.38;
  float densePatch = fbm(v_WorldPos.xz * 1.45 + vec2(4.0, 17.0));
  float patchHeight = mix(0.64, 1.08, valueNoise(floor(v_WorldPos.xz * 4.2) + vec2(v_Var * 19.0, 7.0)));
  float heightFade = 1.0 - smoothstep(patchHeight, patchHeight + 0.12, height);
  float edgeNoise = fbm(v_WorldPos.xz * 6.8 + vec2(v_Var * 19.0 + windT, height * 3.3));
  float tipFray = smoothstep(0.34, patchHeight + 0.08, height);
  float topScatter = valueNoise(v_WorldPos.xz * 28.0 + vec2(v_Uv.x * 17.0, v_Var * 13.0 + windT));
  float sideSign = (v_Uv.x < 0.5) ? -1.0 : 1.0;
  float topLean = sideSign * tipFray * (0.10 + 0.18 * topScatter) + (topScatter - 0.5) * tipFray * 0.18;
  float x = abs(v_Uv.x - (0.5 + topLean));
  float width = mix(0.46, 0.31, pow(height, 1.10));
  width += (edgeNoise - 0.5) * mix(0.014, 0.032, haze);
  width += tipFray * (0.060 + 0.075 * valueNoise(v_WorldPos.xz * 12.0 + v_Var));
  float core = 1.0 - smoothstep(width, width + mix(0.036, 0.070, tipFray), x);
  float edgeDistance = abs(abs(v_Uv.x - 0.5) - (0.36 + 0.10 * topScatter));
  float sideWisp = (1.0 - smoothstep(0.018, 0.125, edgeDistance)) * smoothstep(0.34, 1.0, height);
  float card = max(core, sideWisp * smoothstep(0.26, 0.86, topScatter));
  card *= smoothstep(0.0, 0.040, height) * heightFade;
  float topBreak = valueNoise(v_WorldPos.xz * 34.0 + vec2(v_Uv.x * 29.0, v_Var * 7.0 - windT));
  card *= mix(1.0, smoothstep(0.10, 0.74, topBreak), tipFray * 0.54);

  // Broken straws: short, irregular marks in several directions instead of long comb lines.
  float patchRand = valueNoise(floor(v_WorldPos.xz * 2.8) + vec2(v_Var * 11.0, 3.0));
  float baseRand = valueNoise(v_WorldPos.xz * 9.5 + vec2(v_Var * 29.0, windT));
  float sway = valueNoise(v_WorldPos.xz * 6.5 + vec2(v_Var * 17.0 + windT, height * 2.4));
  float frizzKick = tipFray * (sideSign * mix(8.0, 18.0, topScatter) + mix(-6.0, 6.0, patchRand));
  float dirA = mix(-14.0, 14.0, patchRand) + frizzKick;
  float dirB = mix(12.0, -17.0, valueNoise(floor(v_WorldPos.xz * 3.6) + 9.0)) + frizzKick * 0.72;
  float dirC = mix(-9.0, 19.0, valueNoise(floor(v_WorldPos.xz * 4.5) + 21.0)) + frizzKick * 1.05;
  float arcA = sin(height * 3.14159) * mix(-4.5, 4.5, valueNoise(v_WorldPos.xz * 5.0 + v_Var));
  float arcB = sin(height * 3.14159 * 0.85 + 0.7) * mix(-3.8, 3.8, valueNoise(v_WorldPos.xz * 6.0 + 11.0));
  float arcC = sin(height * 3.14159 * 1.25 + 1.4) * mix(-3.2, 3.2, valueNoise(v_WorldPos.xz * 7.0 + 23.0));
  vec2 uvA = vec2(v_Uv.x * 32.0 + height * dirA + arcA + sway * 1.5, height * 11.0 + v_Var * 7.0);
  vec2 uvB = vec2(v_Uv.x * 44.0 + height * dirB + arcB + sway * 1.1, height * 8.0 + v_Var * 11.0);
  vec2 uvC = vec2(v_Uv.x * 26.0 + height * dirC + arcC - sway * 1.2, height * 13.0 + v_Var * 5.0);
  float strandHeightA = mix(0.46, 1.08, hash21(vec2(floor(uvA.x), floor(v_WorldPos.x * 2.0) + v_Var * 7.0)));
  float strandHeightB = mix(0.42, 1.00, hash21(vec2(floor(uvB.x), floor(v_WorldPos.z * 2.0) + v_Var * 11.0)));
  float strandHeightC = mix(0.38, 0.96, hash21(vec2(floor(uvC.x), floor((v_WorldPos.x + v_WorldPos.z) * 1.4) + v_Var * 5.0)));
  float stripeA = 1.0 - smoothstep(0.070, 0.170, abs(fract(uvA.x) - 0.5));
  float stripeB = 1.0 - smoothstep(0.060, 0.145, abs(fract(uvB.x) - 0.5));
  float stripeC = 1.0 - smoothstep(0.054, 0.132, abs(fract(uvC.x) - 0.5));
  stripeA *= 1.0 - smoothstep(strandHeightA, strandHeightA + 0.12, height);
  stripeB *= 1.0 - smoothstep(strandHeightB, strandHeightB + 0.12, height);
  stripeC *= 1.0 - smoothstep(strandHeightC, strandHeightC + 0.12, height);
  float breakA = smoothstep(0.24, 0.68, valueNoise(uvA * vec2(0.20, 0.74) + windT));
  float breakB = smoothstep(0.28, 0.72, valueNoise(uvB * vec2(0.17, 0.86) - windT * 0.7));
  float breakC = smoothstep(0.32, 0.76, valueNoise(uvC * vec2(0.27, 0.62) + 9.0));
  float straws = max(stripeA * breakA, max(stripeB * breakB * 0.88, stripeC * breakC * 0.76));
  float bladeDensity = smoothstep(0.56, 0.88, densePatch) * smoothstep(0.12, 0.84, height);
  float bladeSeed = valueNoise(v_WorldPos.xz * 13.0 + vec2(v_Var * 21.0, height * 6.0));
  float bladeCurve = sideSign * pow(height, 1.35) * mix(0.06, 0.18, bladeSeed) +
                     sin(height * 3.14159 * mix(0.72, 1.16, bladeSeed)) * mix(-0.10, 0.10, bladeSeed);
  float bladeCenter = 0.5 + bladeCurve + mix(-0.16, 0.16, valueNoise(floor(v_WorldPos.xz * 7.0) + v_Var));
  float bladeWidth = mix(0.054, 0.105, bladeSeed);
  float bladeAccent = 1.0 - smoothstep(bladeWidth, bladeWidth + 0.032, abs(v_Uv.x - bladeCenter));
  bladeAccent *= smoothstep(0.05, 0.24, height) * (1.0 - smoothstep(0.78, 1.0, height));
  straws = max(straws, bladeAccent * bladeDensity * 0.92);
  float grain = valueNoise(v_WorldPos.xz * 43.0 + vec2(v_Var * 31.0 + windT, height * 9.0));
  float fine = saturate(straws * mix(0.90, 1.08, grain));

  // Short grass is thick near the ground and quickly breaks into small tips.
  float baseRough = mix(0.72, 1.18, baseRand);
  float baseBreak = smoothstep(0.18, 0.82, valueNoise(v_WorldPos.xz * 18.0 + vec2(v_Uv.x * 9.0, v_Var * 5.0)));
  float rootMass = smoothstep(0.0, 0.18, height) * (1.0 - smoothstep(0.24, 0.62, height));
  float alpha = card * saturate(fine + bladeAccent * bladeDensity * 0.26 + rootMass * 0.42 * baseRough * mix(0.68, 1.0, baseBreak));

  // Camera haze: soften distant grass sections so they become a thick carpet mass.
  float camHaze = smoothstep(5.0, 22.0, v_ViewDist) * haze;
  alpha = mix(alpha, alpha * 0.72 + card * fine * 0.28, camHaze);
  alpha *= saturate(v_Fade);

  // Stable world-space alpha test: avoids screen-space glitter while still writing depth.
  float dither = valueNoise(v_WorldPos.xz * 72.0 + vec2(height * 17.0, v_Var * 31.0));
  float cutoff = mix(0.18, 0.31, camHaze);
  if (alpha < cutoff + (dither - 0.5) * 0.026) discard;

  vec3 darkGreen = vec3(0.052, 0.135, 0.043);
  vec3 bodyGreen = vec3(0.20, 0.41, 0.10);
  vec3 sunGreen = vec3(0.60, 0.68, 0.20);
  vec3 coolGreen = vec3(0.10, 0.32, 0.13);
  vec3 yellowGreen = vec3(0.43, 0.56, 0.14);
  vec3 color = mix(darkGreen, bodyGreen, smoothstep(0.04, 0.72, height));
  color = mix(color, sunGreen, smoothstep(0.42, 1.0, height) * 0.42);
  float colorNoise = valueNoise(v_WorldPos.xz * 5.2 + vec2(v_Var * 23.0, 8.0));
  float fineColor = valueNoise(v_WorldPos.xz * 38.0 + vec2(height * 4.0, v_Var * 17.0));
  color = mix(color, coolGreen, smoothstep(0.08, 0.34, colorNoise) * 0.34);
  color = mix(color, yellowGreen, smoothstep(0.58, 0.92, colorNoise) * 0.28);
  color *= mix(0.94, 1.07, fineColor);
  color *= u_SpeciesTint;

  if (u_UseAlbedo != 0) {
    vec3 tex = softTexture(v_WorldPos.xz * u_AlbedoUvScale);
    color *= mix(vec3(0.92), tex, 0.46);
  }

  float pockets = smoothstep(0.58, 0.88, densePatch);
  color *= mix(1.06, 0.58, pockets * thick);
  color *= mix(0.96, 1.10, max(fine, bladeAccent * bladeDensity));

  vec3 L = normalize(-u_SunDir);
  float wrap = saturate((dot(vec3(0.0, 1.0, 0.0), L) + 0.45) / 1.45);
  float banded = floor(wrap * 3.0 + 0.5) / 3.0;
  wrap = mix(wrap, banded, saturate(u_StylizedBands));
  vec3 light = u_SunColor * u_SunIntensity;
  color *= (0.34 + 0.66 * wrap) * light;

  // Haze compresses contrast and pushes the carpet toward a soft yellow-green mass.
  vec3 hazeColor = vec3(0.54, 0.66, 0.24) * u_SpeciesTint;
  color = mix(color, hazeColor, camHaze * 0.48);

  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, 1.0);
}
