// Grass fragment shader (alpha cutout)

#version 410 core

in vec2 v_Uv;
in float v_Light;

out vec4 o_Color;

uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main() {
  // Cheap blade fade (keeps it thinner without extra geometry).
  float x = abs(v_Uv.x - 0.5);
  float alpha = 1.0 - smoothstep(0.36, 0.50, x);
  alpha *= smoothstep(0.0, 0.05, v_Uv.y);
  if (alpha < 0.10) discard;

  // Simple gradient color (low quality on purpose).
  vec3 base = mix(vec3(0.10, 0.24, 0.10), vec3(0.18, 0.40, 0.16), v_Uv.y);

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
  o_Color = vec4(color, 1.0);
}
