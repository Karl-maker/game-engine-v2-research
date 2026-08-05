// PBR-ish shader key used by material presets.
//
// Alias of the terrain tessellation control shader.

#version 410 core

layout(vertices = 3) out;

in vec3 v_WorldPos[];
in vec3 v_WorldNormal[];
in vec2 v_Uv[];

out vec3 tc_WorldPos[];
out vec3 tc_WorldNormal[];
out vec2 tc_Uv[];

uniform vec3 u_CameraPos;
uniform vec3 u_CameraForward;

// Distance-based tessellation.
uniform float u_TessNear;  // meters
uniform float u_TessFar;   // meters
uniform float u_TessMin;   // >= 1
uniform float u_TessMax;   // <= 64 (typical)

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float tessFactorAt(vec3 samplePos) {
  vec3 toSample = samplePos - u_CameraPos;
  float distSq = dot(toSample, toSample);
  float invDist = inversesqrt(max(distSq, 1e-6));
  float distMeters = distSq * invDist;

  vec3 viewDir = normalize(u_CameraForward);
  float viewDot = dot(toSample * invDist, viewDir);
  float viewW = smoothstep(0.15, 0.45, viewDot);

  float denom = max(0.001, (u_TessFar - u_TessNear));
  float t = saturate((distMeters - u_TessNear) / denom);
  float tess = mix(u_TessMax, u_TessMin, t);
  tess *= viewW;
  tess = max(1.0, tess);
  tess = clamp(tess, 1.0, 64.0);
  return ceil(tess);
}

void main() {
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  tc_WorldPos[gl_InvocationID] = v_WorldPos[gl_InvocationID];
  tc_WorldNormal[gl_InvocationID] = v_WorldNormal[gl_InvocationID];
  tc_Uv[gl_InvocationID] = v_Uv[gl_InvocationID];

  if (gl_InvocationID == 0) {
    float edge12 = tessFactorAt((v_WorldPos[1] + v_WorldPos[2]) * 0.5);
    float edge20 = tessFactorAt((v_WorldPos[2] + v_WorldPos[0]) * 0.5);
    float edge01 = tessFactorAt((v_WorldPos[0] + v_WorldPos[1]) * 0.5);
    float center = tessFactorAt((v_WorldPos[0] + v_WorldPos[1] + v_WorldPos[2]) * (1.0 / 3.0));

    gl_TessLevelOuter[0] = edge12;
    gl_TessLevelOuter[1] = edge20;
    gl_TessLevelOuter[2] = edge01;
    gl_TessLevelInner[0] = max(center, max(edge12, max(edge20, edge01)));
  }
}
