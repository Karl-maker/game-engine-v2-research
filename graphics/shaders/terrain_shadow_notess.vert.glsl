// Terrain shadow caster vertex shader (non-tess LOD variant)

#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Uv;

uniform mat4 u_Model;
uniform mat4 u_LightViewProj;

void main() {
  vec4 worldPos = u_Model * vec4(a_Position, 1.0);
  gl_Position = u_LightViewProj * worldPos;
}

