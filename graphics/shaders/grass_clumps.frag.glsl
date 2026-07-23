// Grass clumps fragment shader (alpha cutout + textured macro variation)

#version 410 core

in vec2 v_Uv;
in vec3 v_WorldPos;
in float v_Var;
in float v_Fade;

out vec4 o_Color;

uniform sampler2D u_AlbedoTex;
uniform int u_UseAlbedo;
uniform float u_AlbedoUvScale;

uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

uniform vec3 u_SpeciesTint;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  // Ribbon cutout: thin tips, thicker bases, soft edge, then discard.
  float x = abs(v_Uv.x - 0.5);
  float width = mix(0.46, 0.11, v_Uv.y);
  float alpha = 1.0 - smoothstep(width, width + 0.06, x);
  alpha *= smoothstep(0.0, 0.05, v_Uv.y);
  alpha *= saturate(v_Fade);
  if (alpha < 0.40) discard;

  // Base gradient + per-instance variation.
  vec3 baseLo = vec3(0.12, 0.24, 0.10);
  vec3 baseHi = vec3(0.40, 0.62, 0.20);
  vec3 base = mix(baseLo, baseHi, v_Uv.y);
  base *= u_SpeciesTint;
  base *= mix(0.86, 1.14, v_Var);
  // Macro color variation from the grass texture (world-projected).
  if (u_UseAlbedo != 0) {
    vec2 uv = v_WorldPos.xz * u_AlbedoUvScale;
    vec3 t = texture(u_AlbedoTex, uv).rgb;
    base *= mix(vec3(0.85), t, 0.55);
  }

  // Lighting (cheap but coherent): wrap a bit so blades don't go too dark.
  vec3 N = vec3(0.0, 1.0, 0.0);
  vec3 L = normalize(-u_SunDir);
  float ndl = saturate(dot(N, L));
  float wrap = saturate((ndl + 0.35) / 1.35);
  vec3 light = u_SunColor * u_SunIntensity;
  vec3 color = base * (0.18 + 0.82 * wrap) * light;

  // Slight extra darkening toward the base (reads as AO-ish density).
  color *= mix(0.75, 1.0, v_Uv.y);

  // Gamma-ish.
  color = pow(color, vec3(1.0 / 2.2));
  o_Color = vec4(color, 1.0);
}
