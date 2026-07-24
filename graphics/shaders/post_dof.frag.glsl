// Simple depth-of-field post-process (color + depth)

#version 410 core

in vec2 v_Uv;
out vec4 o_Color;

uniform sampler2D u_ColorTex;
uniform sampler2D u_DepthTex;
uniform vec2 u_TexelSize;
uniform float u_Near;
uniform float u_Far;

uniform float u_FocusDistance;
uniform float u_FocusRange;
uniform float u_BlurStrength;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float linearizeDepth(float depth01) {
  float z = depth01 * 2.0 - 1.0;
  return (2.0 * u_Near * u_Far) / max(0.0001, (u_Far + u_Near - z * (u_Far - u_Near)));
}

vec3 sampleColor(vec2 uv) {
  return texture(u_ColorTex, uv).rgb;
}

void main() {
  float d = texture(u_DepthTex, v_Uv).r;
  float z = linearizeDepth(d);

  float range = max(0.05, u_FocusRange);
  float coc = saturate(abs(z - u_FocusDistance) / range) * max(0.0, u_BlurStrength);

  // Blur radius in pixels (kept small for performance).
  float r = clamp(coc * 12.0, 0.0, 12.0);
  vec2 dx = vec2(u_TexelSize.x * r, 0.0);
  vec2 dy = vec2(0.0, u_TexelSize.y * r);

  vec3 c = sampleColor(v_Uv) * 0.20;
  c += sampleColor(v_Uv + dx) * 0.10;
  c += sampleColor(v_Uv - dx) * 0.10;
  c += sampleColor(v_Uv + dy) * 0.10;
  c += sampleColor(v_Uv - dy) * 0.10;
  c += sampleColor(v_Uv + dx + dy) * 0.08;
  c += sampleColor(v_Uv - dx + dy) * 0.08;
  c += sampleColor(v_Uv + dx - dy) * 0.08;
  c += sampleColor(v_Uv - dx - dy) * 0.08;
  c += sampleColor(v_Uv + dx * 2.0) * 0.06;
  c += sampleColor(v_Uv - dx * 2.0) * 0.06;
  c += sampleColor(v_Uv + dy * 2.0) * 0.06;
  c += sampleColor(v_Uv - dy * 2.0) * 0.06;
  c += sampleColor(v_Uv + vec2(dx.x, dy.y) * 1.5) * 0.05;
  c += sampleColor(v_Uv - vec2(dx.x, dy.y) * 1.5) * 0.05;
  c += sampleColor(v_Uv + vec2(dx.x, -dy.y) * 1.5) * 0.05;
  c += sampleColor(v_Uv - vec2(dx.x, -dy.y) * 1.5) * 0.05;

  vec3 sharp = sampleColor(v_Uv);
  vec3 outCol = mix(sharp, c, saturate(coc));
  o_Color = vec4(outCol, 1.0);
}
