#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Uv;
layout(location = 3) in uvec4 a_Joints;
layout(location = 4) in vec4 a_Weights;

uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform bool u_Skinned;
uniform mat4 u_Bones[96];

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_Uv;

void main() {
  mat4 skin = mat4(1.0);
  float weightSum = a_Weights.x + a_Weights.y + a_Weights.z + a_Weights.w;
  if (u_Skinned && weightSum > 0.0001) {
    skin = a_Weights.x * u_Bones[int(a_Joints.x)] +
           a_Weights.y * u_Bones[int(a_Joints.y)] +
           a_Weights.z * u_Bones[int(a_Joints.z)] +
           a_Weights.w * u_Bones[int(a_Joints.w)];
  }

  vec4 localPos = skin * vec4(a_Position, 1.0);
  vec3 localNormal = normalize(mat3(skin) * a_Normal);
  vec4 wp = u_Model * localPos;
  v_WorldPos = wp.xyz;
  v_WorldNormal = normalize(mat3(u_Model) * localNormal);
  v_Uv = a_Uv;
  gl_Position = u_ViewProj * wp;
}
