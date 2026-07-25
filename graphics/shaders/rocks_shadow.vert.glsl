// Instanced rocks shadow caster vertex shader (depth-only)

#version 410 core

layout(location = 0) in vec3 a_Position;

layout(location = 2) in vec3 i_LocalPos;
layout(location = 3) in float i_Scale;
layout(location = 4) in float i_Rot;

uniform mat4 u_LightViewProj;
uniform vec3 u_InstanceOrigin;

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
  vec3 p = R * (a_Position * i_Scale) + u_InstanceOrigin + i_LocalPos;
  gl_Position = u_LightViewProj * vec4(p, 1.0);
}
