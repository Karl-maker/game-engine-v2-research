// Terrain tessellation evaluation shader (demo)
//
// Applies displacement in world space and outputs the final position/normals to the fragment shader.

#version 410 core

layout(triangles, equal_spacing, ccw) in;

in vec3 tc_WorldPos[];
in vec3 tc_WorldNormal[];
in vec2 tc_Uv[];

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_Uv;

uniform mat4 u_ViewProj;
uniform vec3 u_CameraPos;

// Reuse tessellation distance bands for displacement LOD.
uniform float u_TessNear;  // meters
uniform float u_TessFar;   // meters

uniform vec2 u_UvTiling;

uniform sampler2D u_DisplacementTex;
uniform bool u_UseDisplacement;
uniform float u_DisplacementStrength;
uniform int u_DisplacementInvert;

uniform sampler2D u_NormalTex;
uniform bool u_UseNormal;
uniform float u_NormalStrength;

// Uses normal-map "detail" to add/boost displacement amplitude.
uniform float u_NormalDisplacementBoost;
uniform float u_NormalDerivedDisplacementStrength;

// Optional rock layer (tessellation displacement only; matches fragment logic closely).
uniform int u_RockLayerEnabled;
uniform vec2 u_RockUvTiling;
uniform float u_RockNoiseScale;
uniform float u_RockBlendStrength;
uniform sampler2D u_RockDisplacementTex;
uniform bool u_UseRockDisplacement;
uniform float u_RockDisplacementStrength;
uniform sampler2D u_RockNormalTex;
uniform bool u_UseRockNormal;
uniform float u_RockNormalStrength;
uniform float u_RockNormalDerivedDisplacementStrength;

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

void main() {
  vec3 b = gl_TessCoord;

  vec3 pos = tc_WorldPos[0] * b.x + tc_WorldPos[1] * b.y + tc_WorldPos[2] * b.z;
  vec3 N = tc_WorldNormal[0] * b.x + tc_WorldNormal[1] * b.y + tc_WorldNormal[2] * b.z;
  vec2 uv = tc_Uv[0] * b.x + tc_Uv[1] * b.y + tc_Uv[2] * b.z;

  N = normalize(N);

  vec2 uvTiled = uv * u_UvTiling;
  float baseDisp = 0.0;
  if (u_UseDisplacement) {
    float h = texture(u_DisplacementTex, uvTiled).r;
    if (u_DisplacementInvert != 0) h = 1.0 - h;
    baseDisp = (h - 0.5) * u_DisplacementStrength;
  }

  // Add displacement derived from normal-map detail (acts like micro-height).
  if (u_UseNormal && (u_NormalDerivedDisplacementStrength > 0.0 || u_NormalDisplacementBoost > 0.0)) {
    vec3 nTex = texture(u_NormalTex, uvTiled).xyz * 2.0 - 1.0;
    float detail = saturate(length(nTex.xy));  // 0..~1
    float strength = saturate(u_NormalStrength);
    baseDisp += (detail - 0.35) * (u_NormalDerivedDisplacementStrength * strength);
    baseDisp *= (1.0 + detail * u_NormalDisplacementBoost * strength);
  }

  float disp = baseDisp;

  // Optional rock displacement blended by the same style mask as the fragment shader.
  if (u_RockLayerEnabled != 0) {
    float slope = 1.0 - saturate(dot(N, vec3(0.0, 1.0, 0.0)));

    vec2 wp = pos.xz * u_RockNoiseScale;
    vec2 warp = vec2(fbm(wp * 1.3), fbm(wp * 1.7)) - 0.5;
    wp += warp * 0.45;
    float n = fbm(wp * 2.2);
    float mask = saturate((n - 0.45) * 2.2);
    mask = saturate(mask + slope * 0.65);
    mask *= saturate(u_RockBlendStrength);

    vec2 uv2 = uv * u_RockUvTiling + (warp * 0.08);
    float rockDisp = 0.0;
    if (u_UseRockDisplacement) {
      rockDisp = (texture(u_RockDisplacementTex, uv2).r - 0.5) * u_RockDisplacementStrength;
    }
    if (u_UseRockNormal && u_RockNormalDerivedDisplacementStrength > 0.0) {
      vec3 rn = texture(u_RockNormalTex, uv2).xyz * 2.0 - 1.0;
      float detail = saturate(length(rn.xy));
      float strength = saturate(u_RockNormalStrength);
      rockDisp += (detail - 0.35) * (u_RockNormalDerivedDisplacementStrength * strength);
    }

    disp = mix(baseDisp, rockDisp, mask);
  }

  // Distance-based displacement LOD: keep detail near camera, fade far away.
  float distMeters = distance(u_CameraPos, pos);
  float denom = max(0.001, (u_TessFar - u_TessNear));
  float tLod = saturate((distMeters - u_TessNear) / denom);
  float lod = 1.0 - tLod;
  lod = lod * lod;  // ease
  // LOD: kill displacement far from camera (keeps tess/displacement only near the player).
  float farScale = 0.0;
  float dispScale = mix(farScale, 1.0, lod);

  vec3 displacedPos = pos + N * (disp * dispScale);

  v_WorldPos = displacedPos;
  v_WorldNormal = N;
  v_Uv = uv;
  gl_Position = u_ViewProj * vec4(displacedPos, 1.0);
}
