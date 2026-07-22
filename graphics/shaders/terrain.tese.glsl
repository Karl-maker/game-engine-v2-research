// Terrain tessellation evaluation shader (demo)
//
// Applies displacement in world space and outputs the final position/normals to the fragment shader.

#version 410 core

layout(triangles, equal_spacing, ccw) in;

in vec3 tc_WorldPos[];
in vec3 tc_WorldNormal[];
in vec2 tc_Uv[];

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_Uv;

uniform mat4 u_ViewProj;
uniform vec3 u_CameraPos;

uniform vec2 u_UvTiling;

uniform sampler2D u_DisplacementTex;
uniform bool u_UseDisplacement;
uniform float u_DisplacementStrength;

uniform sampler2D u_NormalTex;
uniform bool u_UseNormal;
uniform float u_NormalStrength;

// Uses normal-map "detail" to slightly boost displacement amplitude.
uniform float u_NormalDisplacementBoost;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  const vec3 b = gl_TessCoord;

  vec3 pos = tc_WorldPos[0] * b.x + tc_WorldPos[1] * b.y + tc_WorldPos[2] * b.z;
  vec3 N = tc_WorldNormal[0] * b.x + tc_WorldNormal[1] * b.y + tc_WorldNormal[2] * b.z;
  vec2 uv = tc_Uv[0] * b.x + tc_Uv[1] * b.y + tc_Uv[2] * b.z;

  N = normalize(N);

  float disp = 0.0;
  vec2 uvTiled = uv * u_UvTiling;
  if (u_UseDisplacement) {
    disp = (texture(u_DisplacementTex, uvTiled).r - 0.5) * u_DisplacementStrength;
  }

  if (u_UseNormal && u_NormalDisplacementBoost > 0.0) {
    vec3 nTex = texture(u_NormalTex, uvTiled).xyz * 2.0 - 1.0;
    // How much the normal deviates from "flat" (acts as a micro-detail proxy).
    float detail = saturate(1.0 - abs(nTex.z));
    float strength = saturate(u_NormalStrength * 0.5);
    disp *= (1.0 + detail * u_NormalDisplacementBoost * strength);
  }

  vec3 displacedPos = pos + N * disp;

  v_WorldPos = displacedPos;
  v_WorldNormal = N;
  v_Uv = uv;
  gl_Position = u_ViewProj * vec4(displacedPos, 1.0);
}

