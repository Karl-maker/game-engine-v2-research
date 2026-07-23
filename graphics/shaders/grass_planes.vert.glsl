// Billboard grass planes vertex shader

#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_Uv;
layout(location = 2) in float a_PlaneId;

layout(location = 3) in vec3 i_WorldPos;
layout(location = 4) in float i_Scale;
layout(location = 5) in float i_Rot;
layout(location = 6) in float i_Var;

uniform mat4 u_ViewProj;
uniform vec3 u_CameraPos;
uniform float u_Time;
uniform float u_FadeNear;
uniform float u_FadeFar;
uniform float u_LodTwoPlaneDist;
uniform float u_LodOnePlaneDist;

out vec2 v_Uv;
out vec3 v_WorldPos;
out float v_Var;
out float v_Fade;
out float v_ViewDist;
flat out float v_PlaneId;
flat out float v_PlaneAlive;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

mat3 rotY(float a) {
  float c = cos(a);
  float s = sin(a);
  return mat3(
      c, 0.0, -s,
      0.0, 1.0, 0.0,
      s, 0.0, c);
}

float planeBaseYaw(float planeId) {
  if (planeId < 0.5) return radians(-18.0);
  if (planeId < 1.5) return radians(29.0);
  return radians(72.0);
}

void main() {
  v_Uv = a_Uv;
  v_Var = i_Var;
  v_PlaneId = a_PlaneId;

  float instanceDist = length(u_CameraPos - i_WorldPos);
  float visiblePlanes = 3.0;
  if (instanceDist >= u_LodOnePlaneDist) visiblePlanes = 1.0;
  else if (instanceDist >= u_LodTwoPlaneDist) visiblePlanes = 2.0;
  v_PlaneAlive = (a_PlaneId < visiblePlanes) ? 1.0 : 0.0;

  vec3 local = a_Position;
  float h = saturate(a_Uv.y);
  float tip = h * h;

  float heightScale = mix(0.82, 1.16, fract(i_Var * 17.31 + a_PlaneId * 0.19));
  float widthScale = mix(0.86, 1.18, fract(i_Var * 23.47 + a_PlaneId * 0.37));
  local.y *= heightScale;
  local.x *= widthScale;

  float frizzDir = mix(-1.0, 1.0, fract(i_Var * 41.19 + a_PlaneId * 0.11));
  local.x += sign(local.x + 0.0001) * (0.035 + 0.065 * fract(i_Var * 13.7 + a_PlaneId)) * pow(h, 1.35);
  local.z += frizzDir * (0.015 + 0.045 * fract(i_Var * 29.7 + a_PlaneId * 0.53)) * pow(h, 1.55);
  local.x += sin(h * 3.14159 * mix(0.90, 1.35, fract(i_Var * 9.11)) + i_Var * 6.28318) * 0.030 * tip;
  local.z += sin(h * 4.71239 + i_Var * 8.213 + a_PlaneId) * 0.020 * tip;

  vec3 toCamera = u_CameraPos - i_WorldPos;
  float cameraFacingYaw = atan(toCamera.x, toCamera.z);
  float lodToBillboard = smoothstep(u_LodTwoPlaneDist, u_LodOnePlaneDist, instanceDist);
  float planeYaw = planeBaseYaw(a_PlaneId) + i_Rot;
  planeYaw = mix(planeYaw, cameraFacingYaw, (a_PlaneId < 0.5) ? lodToBillboard : 0.0);

  float swayPhase = dot(i_WorldPos.xz, vec2(0.18, 0.24)) + u_Time * mix(0.55, 0.82, fract(i_Var * 7.73));
  float sway = sin(swayPhase + h * 1.8 + i_Var * 6.28318) * (0.020 + 0.018 * fract(i_Var * 31.0));
  local.x += sway * tip;
  local.z += cos(swayPhase * 0.78 + h * 2.6) * 0.014 * tip;

  vec3 p = rotY(planeYaw) * (local * i_Scale) + i_WorldPos;

  float dist = length(u_CameraPos - p);
  v_ViewDist = dist;
  v_Fade = 1.0 - smoothstep(u_FadeNear, u_FadeFar, dist);
  v_WorldPos = p;
  gl_Position = u_ViewProj * vec4(p, 1.0);
}
