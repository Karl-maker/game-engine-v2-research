// Terrain fragment shader (demo)
//
// Intended mapping from ECS:
// - ShaderComponent.textures -> sampler2D bindings (by slot name)
// - ShaderComponent.parameters -> uniforms like u_BaseColor, u_Roughness, etc.
// - LightComponent (+ TransformComponent) -> packed arrays below
//
// Notes:
// - This repo currently does not compile/link shaders at runtime; this file is an asset stub.

#version 410 core

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_Uv;

out vec4 o_Color;

uniform vec3 u_CameraPos;

// Material-ish controls (example names).
uniform vec4 u_BaseColor;
uniform float u_Roughness;
uniform float u_Metallic;

// Optional albedo texture.
uniform sampler2D u_Albedo;
uniform bool u_UseAlbedo = false;

// Minimal packed light data.
// type: 0=Directional, 1=Point, 2=Spot (matches LightComponent::Type order in C++).
const int MAX_LIGHTS = 16;
uniform int u_LightCount;
uniform int u_LightType[MAX_LIGHTS];
uniform vec3 u_LightPos[MAX_LIGHTS];
uniform vec3 u_LightDir[MAX_LIGHTS];
uniform vec3 u_LightColor[MAX_LIGHTS];
uniform float u_LightIntensity[MAX_LIGHTS];
uniform float u_LightRange[MAX_LIGHTS];

vec3 shadeLambert(vec3 albedo, vec3 normal, vec3 viewDir) {
  vec3 result = vec3(0.0);

  for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; ++i) {
    vec3 L = vec3(0.0);
    float attenuation = 1.0;

    if (u_LightType[i] == 0) {
      // Directional: direction points "from light" in world-space.
      L = normalize(-u_LightDir[i]);
    } else {
      // Point/spot: position is world-space.
      vec3 toLight = u_LightPos[i] - v_WorldPos;
      float dist = length(toLight);
      if (dist > 0.0001) {
        L = toLight / dist;
      }

      float range = max(u_LightRange[i], 0.0001);
      float falloff = clamp(1.0 - (dist / range), 0.0, 1.0);
      attenuation = falloff * falloff;
    }

    float NdotL = max(dot(normal, L), 0.0);
    vec3 light = u_LightColor[i] * u_LightIntensity[i] * attenuation;
    result += albedo * light * NdotL;
  }

  // Tiny ambient term.
  result += albedo * 0.03;
  return result;
}

void main() {
  vec3 N = normalize(v_WorldNormal);
  vec3 V = normalize(u_CameraPos - v_WorldPos);

  vec3 albedo = u_BaseColor.rgb;
  if (u_UseAlbedo) {
    albedo *= texture(u_Albedo, v_Uv).rgb;
  }

  vec3 color = shadeLambert(albedo, N, V);
  o_Color = vec4(color, 1.0);
}
