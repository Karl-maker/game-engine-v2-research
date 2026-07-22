// Terrain tessellation control shader (demo)
//
// Builds triangle patches and picks a tessellation factor based on camera distance.

#version 410 core

layout(vertices = 3) out;

in vec3 v_WorldPos[];
in vec3 v_WorldNormal[];
in vec2 v_Uv[];

out vec3 tc_WorldPos[];
out vec3 tc_WorldNormal[];
out vec2 tc_Uv[];

uniform vec3 u_CameraPos;

// Distance-based tessellation.
uniform float u_TessNear;  // meters
uniform float u_TessFar;   // meters
uniform float u_TessMin;   // >= 1
uniform float u_TessMax;   // <= 64 (typical)

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  tc_WorldPos[gl_InvocationID] = v_WorldPos[gl_InvocationID];
  tc_WorldNormal[gl_InvocationID] = v_WorldNormal[gl_InvocationID];
  tc_Uv[gl_InvocationID] = v_Uv[gl_InvocationID];

  if (gl_InvocationID == 0) {
    vec3 center = (v_WorldPos[0] + v_WorldPos[1] + v_WorldPos[2]) * (1.0 / 3.0);
    float distMeters = distance(u_CameraPos, center);
    float denom = max(0.001, (u_TessFar - u_TessNear));
    float t = saturate((distMeters - u_TessNear) / denom);
    float tess = mix(u_TessMax, u_TessMin, t);
    tess = clamp(tess, 1.0, 64.0);

    gl_TessLevelOuter[0] = tess;
    gl_TessLevelOuter[1] = tess;
    gl_TessLevelOuter[2] = tess;
    gl_TessLevelInner[0] = tess;
  }
}

