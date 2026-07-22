// Grass vertex shader (instanced billboards)

#version 410 core

layout(location = 0) in vec2 a_LocalPos;  // x = blade width, y = blade height (0..1)
layout(location = 1) in vec2 a_Uv;

layout(location = 2) in vec3 i_WorldPos;
layout(location = 3) in float i_Scale;
layout(location = 4) in float i_Rot;

uniform mat4 u_ViewProj;

uniform vec3 u_CamRight;
uniform vec3 u_CamForward;

out vec2 v_Uv;
out float v_Light;
out float v_Var;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  v_Uv = a_Uv;

  vec3 up = vec3(0.0, 1.0, 0.0);

  // Rotate billboard basis around up so blades vary.
  float c = cos(i_Rot);
  float s = sin(i_Rot);
  vec3 right = normalize(u_CamRight * c + u_CamForward * s);
  vec3 fwd = normalize(cross(up, right));

  // Per-instance bend direction and amount (static; no swaying).
  float id = fract(sin(dot(i_WorldPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
  v_Var = id;
  float leanAngle = (id * 2.0 - 1.0) * 0.85;
  vec3 leanDir = normalize(right * cos(leanAngle) + fwd * sin(leanAngle));

  // Gentle constant lean. Stronger near the tips.
  float tip = a_LocalPos.y * a_LocalPos.y;
  float bend = (0.08 + 0.22 * id) * tip;

  vec3 pos = i_WorldPos;
  // Slight curvature across the blade width.
  float curve = sin((a_LocalPos.y + id) * 3.14159) * 0.010 * i_Scale;
  pos += right * (a_LocalPos.x * i_Scale + curve);
  pos += up * (a_LocalPos.y * i_Scale);
  pos += leanDir * (bend * i_Scale);

  // Fake lighting term from up-facing normal.
  v_Light = saturate(0.35 + 0.65 * a_LocalPos.y);

  gl_Position = u_ViewProj * vec4(pos, 1.0);
}
