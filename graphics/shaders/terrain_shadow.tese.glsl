// Terrain shadow caster tessellation evaluation shader (demo)
//
// Depth-only output. This intentionally does not sample displacement textures to keep the shadow pass cheap.

#version 410 core

layout(triangles, equal_spacing, ccw) in;

in vec3 tc_WorldPos[];
in vec3 tc_WorldNormal[];
in vec2 tc_Uv[];

uniform mat4 u_LightViewProj;

void main() {
  vec3 b = gl_TessCoord;

  vec3 pos = tc_WorldPos[0] * b.x + tc_WorldPos[1] * b.y + tc_WorldPos[2] * b.z;
  gl_Position = u_LightViewProj * vec4(pos, 1.0);
}

