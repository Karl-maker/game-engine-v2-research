// Grass clumps vertex shader (GPU-instanced ribbons with wind + interaction)

#version 410 core

layout(location = 0) in vec3 a_Position; // local clump space; y in 0..1
layout(location = 1) in vec2 a_Uv;
layout(location = 2) in float a_PlaneId;

layout(location = 3) in vec3 i_WorldPos;
layout(location = 4) in float i_Scale;
layout(location = 5) in float i_Rot;
layout(location = 6) in float i_Var;

uniform mat4 u_ViewProj;
uniform vec3 u_CameraPos;
uniform float u_Time;

uniform vec2 u_WindDirXZ;
uniform float u_WindSpeed;
uniform float u_WindStrength;
uniform float u_BladeBendStrength;
uniform float u_BladeCurveStrength;
uniform float u_BladeTwistStrength;

uniform float u_FadeNear;
uniform float u_FadeFar;

uniform int u_InteractionCount;
uniform vec4 u_InteractionsPosRad[4]; // xyz=pos, w=radius
uniform float u_InteractionsStrength[4];

out vec2 v_Uv;
out vec3 v_WorldPos;
out float v_Var;
out float v_Fade;
out float v_ViewDist;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

mat3 rotY(float a) {
  float c = cos(a);
  float s = sin(a);
  return mat3(
    c, 0.0, -s,
    0.0, 1.0, 0.0,
    s, 0.0,  c
  );
}

void main() {
  v_Uv = a_Uv;
  v_Var = i_Var;

  mat3 R = rotY(i_Rot);
  vec3 p = R * (a_Position * i_Scale) + i_WorldPos;

  // Fade by camera distance (keep transitions smooth; CPU does coarse chunk culling).
  float dist = length(u_CameraPos - p);
  v_ViewDist = dist;
  v_Fade = 1.0 - smoothstep(u_FadeNear, u_FadeFar, dist);

  // Wind: treat it as a spatial field (waves), not independent wiggles.
  vec2 windDir = normalize(u_WindDirXZ);
  float phase = dot(p.xz, windDir) * 0.35 + u_Time * u_WindSpeed;
  float phase2 = (p.x + p.z) * 0.12 + u_Time * (u_WindSpeed * 0.72);
  float wave = sin(phase) + 0.6 * sin(phase2);

  float h = saturate(a_Uv.y);
  float tip = h * h;
  float gust = (0.30 + 0.70 * i_Var);

  vec2 bendDir = normalize(vec2(sin(i_Var * 31.416 + 1.7), cos(i_Var * 18.849 + 0.9)));
  vec2 curveDir = vec2(-bendDir.y, bendDir.x);
  p.xz += bendDir * u_BladeBendStrength * (0.15 + 0.85 * tip) * i_Scale;
  p.xz += curveDir * sin(h * 3.14159 * 1.25 + i_Var * 6.28318) * u_BladeCurveStrength * tip * i_Scale;
  p.xz += bendDir * cos(h * 6.28318 + i_Var * 12.56636) * u_BladeTwistStrength * tip * 0.25 * i_Scale;

  vec2 windOffset = windDir * (wave * 0.12) * (u_WindStrength * gust) * tip;
  p.xz += windOffset;

  // Interaction spheres (player/actors): bend away.
  for (int i = 0; i < u_InteractionCount && i < 4; ++i) {
    vec3 ip = u_InteractionsPosRad[i].xyz;
    float rad = max(0.01, u_InteractionsPosRad[i].w);
    vec3 d = p - ip;
    float dl = length(d);
    float k = saturate(1.0 - (dl / rad)) * u_InteractionsStrength[i];
    if (k > 0.0001) {
      vec2 away = normalize(d.xz + vec2(0.0001, 0.0001));
      p.xz += away * (k * 0.42) * tip;
      p.y -= (k * 0.08) * tip;
    }
  }

  v_WorldPos = p;
  gl_Position = u_ViewProj * vec4(p, 1.0);
}
