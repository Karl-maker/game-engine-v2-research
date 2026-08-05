#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Uv;

uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform float u_Time;
uniform float u_WaveHeight = 0.0;
uniform float u_WaveScale = 0.085;
uniform float u_WaveSpeed = 0.28;
uniform vec2 u_WaveDirection = vec2(1.0, 0.2);
uniform float u_SecondaryWaveHeight = 0.0;
uniform float u_SecondaryWaveScale = 0.16;
uniform float u_SecondaryWaveSpeed = 0.18;
uniform vec2 u_SecondaryWaveDirection = vec2(-0.35, 1.0);

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_Uv;

vec2 safeDir(vec2 v) {
  float lenSq = dot(v, v);
  if (lenSq < 1e-5) return vec2(1.0, 0.0);
  return v * inversesqrt(lenSq);
}

float waveOffset(vec2 worldXZ) {
  if (abs(u_WaveHeight) < 1e-5 && abs(u_SecondaryWaveHeight) < 1e-5) return 0.0;

  vec2 dirA = safeDir(u_WaveDirection);
  vec2 dirB = safeDir(u_SecondaryWaveDirection);
  float phaseA = dot(worldXZ, dirA) * u_WaveScale + u_Time * u_WaveSpeed;
  float phaseB = dot(worldXZ, dirB) * u_SecondaryWaveScale + u_Time * u_SecondaryWaveSpeed;
  return sin(phaseA) * u_WaveHeight + sin(phaseB) * u_SecondaryWaveHeight;
}

void main() {
  vec4 worldPos = u_Model * vec4(a_Position, 1.0);
  worldPos.y += waveOffset(worldPos.xz);
  v_WorldPos = worldPos.xyz;
  v_WorldNormal = mat3(u_Model) * a_Normal;
  v_Uv = a_Uv;
  gl_Position = u_ViewProj * worldPos;
}
