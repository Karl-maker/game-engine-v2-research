// PBR-ish shader key used by material presets.
//
// Today this is a thin alias of the terrain demo shader so presets like
// `materials::presets::HighQualityDirtRockGrassLayer()` render without needing
// per-scene overrides.
//
// If/when a true mesh PBR shader is added, this file can be replaced and the
// presets can keep their stable `shaders/pbr` key.

#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Uv;

uniform mat4 u_Model;
uniform mat4 u_ViewProj;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_Uv;

void main() {
  vec4 worldPos = u_Model * vec4(a_Position, 1.0);
  v_WorldPos = worldPos.xyz;
  v_WorldNormal = mat3(u_Model) * a_Normal;
  v_Uv = a_Uv;
#if defined(HAS_TESSELLATION)
  // When tessellation is enabled, pass world-space positions through the patch stages.
  gl_Position = worldPos;
#else
  gl_Position = u_ViewProj * worldPos;
#endif
}

