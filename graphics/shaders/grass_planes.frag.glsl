// Billboard grass planes fragment shader

#version 410 core

in vec2 v_Uv;
in vec3 v_WorldPos;
in float v_Var;
in float v_Fade;
in float v_ViewDist;
flat in float v_PlaneId;
flat in float v_PlaneAlive;

out vec4 o_Color;

uniform sampler2D u_GrassTex0;
uniform sampler2D u_GrassTex1;
uniform sampler2D u_GrassTex2;
uniform sampler2D u_GrassTex3;
uniform sampler2D u_GrassTex4;
uniform sampler2D u_GrassTex5;

uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;
uniform vec3 u_SpeciesTint;
uniform float u_CarpetHaze;
uniform float u_StylizedBands;
uniform float u_CarpetThickness;
uniform float u_Time;
uniform float u_BottomFade;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 345.45));
  p += dot(p, p + 34.345);
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

vec4 sampleGrassTexture(float selector, vec2 uv) {
  if (selector < 0.34) return texture(u_GrassTex0, uv);
  if (selector < 0.62) return texture(u_GrassTex1, uv);
  if (selector < 0.73) return texture(u_GrassTex2, uv);
  if (selector < 0.83) return texture(u_GrassTex3, uv);
  if (selector < 0.92) return texture(u_GrassTex4, uv);
  return texture(u_GrassTex5, uv);
}

vec4 softGrassSample(float selector, vec2 uv, float blurAmount) {
  vec2 blur = vec2(0.020, 0.035) * blurAmount;
  vec4 c = sampleGrassTexture(selector, uv) * 0.56;
  c += sampleGrassTexture(selector, uv + blur) * 0.22;
  c += sampleGrassTexture(selector, uv - blur * vec2(0.7, 1.0)) * 0.22;
  return c;
}

void main() {
  if (v_PlaneAlive < 0.5) discard;

  float nearPatchChance = fract(v_Var * 17.13 + v_PlaneId * 0.07);
  float texSelector = nearPatchChance;
  float blurAmount = mix(0.35, 1.0, smoothstep(4.0, 24.0, v_ViewDist) * saturate(u_CarpetHaze));

  vec2 texUv = vec2(
      mix(0.12, 0.88, v_Uv.x) + (valueNoise(v_WorldPos.xz * 2.2 + v_PlaneId) - 0.5) * 0.03,
      mix(0.02, 0.98, v_Uv.y));
  texUv.y = pow(texUv.y, 0.92);

  vec4 texel = softGrassSample(texSelector, texUv, blurAmount);
  float greenDominance = texel.g - max(texel.r, texel.b) * 0.78;
  float brightness = dot(texel.rgb, vec3(0.2126, 0.7152, 0.0722));
  float textureMask = max(texel.a, smoothstep(0.02, 0.30, greenDominance + brightness * 0.18));

  float bottomFade = smoothstep(0.0, max(0.05, u_BottomFade), v_Uv.y);
  float topFeather = 1.0 - smoothstep(0.78, 1.04, v_Uv.y + (valueNoise(v_WorldPos.xz * 7.0 + v_Var) - 0.5) * 0.16);
  float sideFeather = 1.0 - smoothstep(0.42, 0.56, abs(v_Uv.x - 0.5));
  float mask = textureMask * bottomFade * topFeather * mix(0.84, 1.0, sideFeather);

  float heightNoise = valueNoise(v_WorldPos.xz * 5.4 + vec2(v_Var * 13.0, 9.0));
  float pocketNoise = valueNoise(v_WorldPos.xz * 11.0 + vec2(v_Var * 29.0, u_Time * 0.02));
  float bladeAccent = smoothstep(0.78, 0.96, pocketNoise) * smoothstep(0.16, 0.86, v_Uv.y);
  float bladeStripe = 1.0 - smoothstep(0.05, 0.17, abs(v_Uv.x - (0.5 + (heightNoise - 0.5) * 0.16 * v_Uv.y)));
  mask = max(mask, bladeAccent * bladeStripe * 0.72 * topFeather);

  float aa = fwidth(mask) * 1.8 + 0.02;
  float alpha = smoothstep(0.28 - aa, 0.28 + aa, mask);
  alpha *= saturate(v_Fade);
  if (alpha < 0.05) discard;

  vec3 base = texel.rgb;
  vec3 darkGreen = vec3(0.14, 0.20, 0.07);
  vec3 midGreen = vec3(0.42, 0.54, 0.18);
  vec3 sunGreen = vec3(0.78, 0.82, 0.36);
  float hueNoise = valueNoise(v_WorldPos.xz * 3.2 + vec2(v_Var * 19.0, 4.0));
  vec3 grade = mix(darkGreen, midGreen, smoothstep(0.02, 0.80, v_Uv.y));
  grade = mix(grade, sunGreen, smoothstep(0.56, 1.0, v_Uv.y) * 0.36);
  grade *= mix(vec3(0.90, 0.96, 0.88), vec3(1.08, 1.03, 0.92), hueNoise);
  vec3 color = base * grade * u_SpeciesTint;

  float selfShadow = smoothstep(0.0, 0.22, 1.0 - v_Uv.y);
  selfShadow += smoothstep(0.64, 0.92, pocketNoise) * 0.28;
  selfShadow += smoothstep(0.40, 0.70, abs(v_Uv.x - 0.5)) * 0.10;
  color *= 1.0 - selfShadow * mix(0.32, 0.52, saturate(u_CarpetThickness));

  vec3 L = normalize(-u_SunDir);
  float lambert = saturate(dot(vec3(0.0, 1.0, 0.0), L) * 0.72 + 0.48);
  float banded = floor(lambert * 4.0 + 0.5) / 4.0;
  lambert = mix(lambert, banded, saturate(u_StylizedBands));
  color *= (0.42 + 0.58 * lambert) * u_SunColor * u_SunIntensity;

  float haze = smoothstep(6.0, 28.0, v_ViewDist) * saturate(u_CarpetHaze);
  vec3 hazeColor = vec3(0.67, 0.72, 0.34) * u_SpeciesTint;
  color = mix(color, hazeColor, haze * 0.34);

  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, alpha);
}
