// Grass fragment shader (alpha cutout)

#version 410 core

in vec2 v_Uv;
in float v_Light;
in float v_Var;

out vec4 o_Color;

uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  // Cheap blade fade (keeps it thin without extra geometry).
  float x = abs(v_Uv.x - 0.5);
  float width = mix(0.40, 0.12, v_Uv.y);
  float alpha = 1.0 - smoothstep(width, width + 0.06, x);
  alpha *= smoothstep(0.0, 0.06, v_Uv.y);
  if (alpha < 0.05) discard;

  // Simple gradient color (low quality on purpose).
  vec3 baseLo = vec3(0.16, 0.32, 0.12);
  vec3 baseHi = vec3(0.36, 0.58, 0.18);
  vec3 base = mix(baseLo, baseHi, v_Uv.y);
  base *= mix(0.85, 1.10, v_Var);
  // Slight yellowish tint variation.
  base += vec3(0.08, 0.06, 0.00) * (v_Var - 0.5) * 0.25;

  // Lighting (simple): up-ish normal, directional sun.
  vec3 N = vec3(0.0, 1.0, 0.0);
  vec3 L = normalize(-u_SunDir);
  float ndl = saturate(dot(N, L));
  vec3 light = u_SunColor * u_SunIntensity;
  vec3 color = base * (0.22 + 0.78 * ndl) * light;

  // Extra gradient + subtle self-shadow.
  color *= v_Light;

  // Gamma-ish.
  color = pow(color, vec3(1.0 / 2.2));
  // Slight transparency so dense patches blend a bit.
  o_Color = vec4(color, alpha * 0.85);
}
