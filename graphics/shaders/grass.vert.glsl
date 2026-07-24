// Unified grass vertex shader:
// - Mode 0: clump ribbons (wind + interaction)
// - Mode 1: billboard planes (3 planes + LOD + gentle GPU sway)
//
// Attribute layout matches the shared grass VAO:
//   0: position (vec3)
//   1: uv       (vec2)
//   2: planeId  (float)  [used by mode 1, ignored by mode 0]
//   3..6: instance data

#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_Uv;
layout(location = 2) in float a_PlaneId;

layout(location = 3) in vec3 i_WorldPos;
layout(location = 4) in float i_Scale;
layout(location = 5) in float i_Rot;
layout(location = 6) in float i_Var;

uniform mat4 u_ViewProj;
uniform vec3 u_CameraPos;
uniform float u_Time;

uniform int u_GrassMode; // 0=clumps, 1=planes

// Clumps-only controls (ignored in plane mode).
uniform vec2 u_WindDirXZ;
uniform float u_WindSpeed;
uniform float u_WindStrength;
uniform float u_BladeBendStrength;
uniform float u_BladeCurveStrength;
uniform float u_BladeTwistStrength;
uniform int u_InteractionCount;
uniform vec4 u_InteractionsPosRad[4];
uniform float u_InteractionsStrength[4];

// Shared fade.
uniform float u_FadeNear;
uniform float u_FadeFar;

// Planes-only LOD.
uniform float u_LodTwoPlaneDist;
uniform float u_LodOnePlaneDist;

out vec2 v_Uv;
out vec3 v_WorldPos;
out float v_Var;
out float v_Fade;
out float v_ViewDist;
flat out float v_PlaneId;
flat out float v_PlaneAlive;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

mat3 rotY(float a) {
  float c = cos(a);
  float s = sin(a);
  return mat3(
    c, 0.0, -s,
    0.0, 1.0, 0.0,
    s, 0.0,  c
  );
}

float planeBaseYaw(float planeId) {
  if (planeId < 0.5) return radians(-18.0);
  if (planeId < 1.5) return radians(29.0);
  return radians(72.0);
}

void main() {
  v_Uv = a_Uv;
  v_Var = i_Var;

  vec3 p = vec3(0.0);
  float planeAlive = 1.0;
  float planeIdOut = 0.0;

  if (u_GrassMode == 1) {
    // Billboard planes mode.
    planeIdOut = a_PlaneId;

    float instanceDist = length(u_CameraPos - i_WorldPos);
    float visiblePlanes = 3.0;
    if (instanceDist >= u_LodOnePlaneDist) visiblePlanes = 1.0;
    else if (instanceDist >= u_LodTwoPlaneDist) visiblePlanes = 2.0;
    planeAlive = (a_PlaneId < visiblePlanes) ? 1.0 : 0.0;

    vec3 local = a_Position;
    float h = saturate(a_Uv.y);
    float tip = h * h;

    // Lower, tighter grass silhouette. Instance scale controls overall size.
    float heightScale = mix(0.66, 0.98, fract(i_Var * 17.31 + a_PlaneId * 0.19));
    float widthScale = mix(0.88, 1.16, fract(i_Var * 23.47 + a_PlaneId * 0.37));
    local.y *= heightScale;
    local.x *= widthScale;

    // Frizz/strand breakup near the top.
    float frizzDir = mix(-1.0, 1.0, fract(i_Var * 41.19 + a_PlaneId * 0.11));
    local.x += sign(local.x + 0.0001) * (0.028 + 0.052 * fract(i_Var * 13.7 + a_PlaneId)) * pow(h, 1.35);
    local.z += frizzDir * (0.012 + 0.036 * fract(i_Var * 29.7 + a_PlaneId * 0.53)) * pow(h, 1.55);
    local.x += sin(h * 3.14159 * mix(0.90, 1.35, fract(i_Var * 9.11)) + i_Var * 6.28318) * 0.024 * tip;
    local.z += sin(h * 4.71239 + i_Var * 8.213 + a_PlaneId) * 0.016 * tip;

    // LOD: in far distance, only plane 0 faces camera.
    vec3 toCamera = u_CameraPos - i_WorldPos;
    float cameraFacingYaw = atan(toCamera.x, toCamera.z);
    float lodToBillboard = smoothstep(u_LodTwoPlaneDist, u_LodOnePlaneDist, instanceDist);
    float planeYaw = planeBaseYaw(a_PlaneId) + i_Rot;
    planeYaw = mix(planeYaw, cameraFacingYaw, (a_PlaneId < 0.5) ? lodToBillboard : 0.0);

    // Gentle GPU sway (no wind settings).
    float swayPhase = dot(i_WorldPos.xz, vec2(0.18, 0.24)) + u_Time * mix(0.55, 0.82, fract(i_Var * 7.73));
    float sway = sin(swayPhase + h * 1.8 + i_Var * 6.28318) * (0.016 + 0.014 * fract(i_Var * 31.0));
    local.x += sway * tip;
    local.z += cos(swayPhase * 0.78 + h * 2.6) * 0.011 * tip;

    p = rotY(planeYaw) * (local * i_Scale) + i_WorldPos;
  } else {
    // Clump ribbons mode.
    planeIdOut = 0.0;
    planeAlive = 1.0;

    mat3 R = rotY(i_Rot);
    p = R * (a_Position * i_Scale) + i_WorldPos;

    // Wind: spatial field (waves), not independent wiggles.
    vec2 windDir = normalize(u_WindDirXZ);
    float phase = dot(p.xz, windDir) * 0.35 + u_Time * u_WindSpeed;
    float phase2 = (p.x + p.z) * 0.12 + u_Time * (u_WindSpeed * 0.72);
    float wave = sin(phase) + 0.6 * sin(phase2);

    float h = saturate(a_Uv.y);
    float tip = h * h;
    float gust = (0.30 + 0.70 * i_Var);

    vec2 bendDir = normalize(vec2(sin(i_Var * 31.416 + 1.7), cos(i_Var * 18.849 + 0.9)));
    vec2 curveDir = vec2(-bendDir.y, bendDir.x);
    p.xz += bendDir * u_BladeBendStrength * (0.15 + 0.85 * tip) * i_Scale;
    p.xz += curveDir * sin(h * 3.14159 * 1.25 + i_Var * 6.28318) * u_BladeCurveStrength * tip * i_Scale;
    p.xz += bendDir * cos(h * 6.28318 + i_Var * 12.56636) * u_BladeTwistStrength * tip * 0.25 * i_Scale;

    vec2 windOffset = windDir * (wave * 0.12) * (u_WindStrength * gust) * tip;
    p.xz += windOffset;

    // Interaction spheres (player/actors): bend away.
    for (int i = 0; i < u_InteractionCount && i < 4; ++i) {
      vec3 ip = u_InteractionsPosRad[i].xyz;
      float rad = max(0.01, u_InteractionsPosRad[i].w);
      vec3 d = p - ip;
      float dl = length(d);
      float k = saturate(1.0 - (dl / rad)) * u_InteractionsStrength[i];
      if (k > 0.0001) {
        vec2 away = normalize(d.xz + vec2(0.0001, 0.0001));
        p.xz += away * (k * 0.42) * tip;
        p.y -= (k * 0.08) * tip;
      }
    }
  }

  // Fade by camera distance (keep transitions smooth; CPU does coarse chunk culling).
  float dist = length(u_CameraPos - p);
  v_ViewDist = dist;
  v_Fade = 1.0 - smoothstep(u_FadeNear, u_FadeFar, dist);

  v_PlaneId = planeIdOut;
  v_PlaneAlive = planeAlive;
  v_WorldPos = p;
  gl_Position = u_ViewProj * vec4(p, 1.0);
}

