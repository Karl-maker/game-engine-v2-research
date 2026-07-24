// Camera motion blur post-process (color + depth + prev view-proj)

#version 410 core

in vec2 v_Uv;
out vec4 o_Color;

uniform sampler2D u_ColorTex;
uniform sampler2D u_DepthTex;
uniform vec2 u_TexelSize;

uniform mat4 u_InvViewProj;
uniform mat4 u_PrevViewProj;

uniform float u_Strength;
uniform float u_MaxBlurPixels;
uniform int u_Samples;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  vec3 base = texture(u_ColorTex, v_Uv).rgb;

  float s = max(0.0, u_Strength);
  int taps = clamp(u_Samples, 1, 24);
  if (s <= 0.0001 || taps <= 1) {
    o_Color = vec4(base, 1.0);
    return;
  }

  float depth01 = texture(u_DepthTex, v_Uv).r;
  vec4 ndc = vec4(v_Uv * 2.0 - 1.0, depth01 * 2.0 - 1.0, 1.0);
  vec4 world = u_InvViewProj * ndc;
  world /= max(0.000001, world.w);

  vec4 prevClip = u_PrevViewProj * vec4(world.xyz, 1.0);
  prevClip /= max(0.000001, prevClip.w);

  vec2 curNdc = ndc.xy;
  vec2 velUv = (curNdc - prevClip.xy) * 0.5;

  float velPixels = length(velUv / max(vec2(1e-6), u_TexelSize));
  float maxPx = max(0.0, u_MaxBlurPixels);
  float scale = (velPixels <= 1e-4) ? 0.0 : min(1.0, maxPx / velPixels);
  vec2 v = velUv * s * scale;

  vec3 sum = vec3(0.0);
  for (int i = 0; i < taps; ++i) {
    float t = (taps == 1) ? 0.0 : (float(i) / float(taps - 1) - 0.5);
    sum += texture(u_ColorTex, v_Uv + v * t).rgb;
  }

  o_Color = vec4(sum / float(taps), 1.0);
}

