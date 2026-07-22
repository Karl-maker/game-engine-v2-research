// Instanced rocks vertex shader

#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

layout(location = 2) in vec3 i_WorldPos;
layout(location = 3) in float i_Scale;
layout(location = 4) in float i_Rot;

uniform mat4 u_ViewProj;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;

mat3 rotY(float a) {
  float c = cos(a);
  float s = sin(a);
  return mat3(
    c, 0.0, -s,
    0.0, 1.0, 0.0,
    s, 0.0, c
  );
}

void main() {
  mat3 R = rotY(i_Rot);
  vec3 p = R * (a_Position * i_Scale) + i_WorldPos;
  vec3 n = normalize(R * a_Normal);

  v_WorldPos = p;
  v_WorldNormal = n;
  gl_Position = u_ViewProj * vec4(p, 1.0);
}

