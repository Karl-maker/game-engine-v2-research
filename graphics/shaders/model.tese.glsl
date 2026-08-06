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
uniform float u_TessNear;
uniform float u_TessFar;

uniform sampler2D u_DisplacementTex;
uniform bool u_UseDisplacement;
uniform float u_DisplacementStrength;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  vec3 b = gl_TessCoord;

  vec3 pos = tc_WorldPos[0] * b.x + tc_WorldPos[1] * b.y + tc_WorldPos[2] * b.z;
  vec3 N = normalize(tc_WorldNormal[0] * b.x + tc_WorldNormal[1] * b.y + tc_WorldNormal[2] * b.z);
  vec2 uv = tc_Uv[0] * b.x + tc_Uv[1] * b.y + tc_Uv[2] * b.z;

  float disp = 0.0;
  if (u_UseDisplacement) {
    disp = (texture(u_DisplacementTex, uv).r - 0.5) * u_DisplacementStrength;
  }

  float distMeters = distance(u_CameraPos, pos);
  float denom = max(0.001, (u_TessFar - u_TessNear));
  float t = saturate((distMeters - u_TessNear) / denom);
  float lod = 1.0 - t;
  lod *= lod;

  vec3 displacedPos = pos + N * (disp * lod);

  v_WorldPos = displacedPos;
  v_WorldNormal = N;
  v_Uv = uv;
  gl_Position = u_ViewProj * vec4(displacedPos, 1.0);
}
