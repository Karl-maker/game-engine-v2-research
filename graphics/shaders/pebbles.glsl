// Pebbles layer (shader snippet)
//
// This file is intended to be included by other shaders via the renderer's
// tiny `#include "path"` preprocessor.

float hash12(vec2 p);
float valueNoise(vec2 p);
float fbm(vec2 p);
float saturate(float x);

// A lightweight "cellular-ish" pebble mask on the XZ plane.
// Returns:
// - mask: 0..1 pebble coverage
// - edge: 0..1 where 1 is near pebble edge (for grime/darkening)
// - height: 0..1 (pebble height field)
vec3 pebbleMask(vec2 worldXZ, float scale, float density) {
  // Scale up into "pebble cells".
  vec2 p = worldXZ * scale;
  vec2 cell = floor(p);
  vec2 f = fract(p);

  float bestDist = 10.0;
  float bestId = 0.0;
  vec2 bestD = vec2(0.0);

  // Search neighboring cells for closest center.
  for (int j = -1; j <= 1; ++j) {
    for (int i = -1; i <= 1; ++i) {
      vec2 c = cell + vec2(float(i), float(j));
      float rnd = hash12(c);
      // Random center and slight anisotropy.
      vec2 center = vec2(hash12(c + 3.1), hash12(c + 7.7));
      vec2 d = (vec2(float(i), float(j)) + center) - f;

      // Per-cell rotation + anisotropy to vary pebble shapes.
      float ang = hash12(c + 11.1) * 6.2831853;
      float ca = cos(ang);
      float sa = sin(ang);
      mat2 R = mat2(ca, -sa, sa, ca);
      float aniso = mix(0.65, 1.45, hash12(c + 5.5));
      vec2 d2 = R * d;
      d2 *= vec2(aniso, 1.0 / aniso);

      float dist = dot(d2, d2);
      if (dist < bestDist) {
        bestDist = dist;
        bestId = rnd;
        bestD = d2;
      }
    }
  }

  // Pebble radius varies a bit per cell.
  float r = mix(0.18, 0.42, bestId);
  // Density is treated as a probability of cell containing a pebble.
  float present = step(1.0 - density, bestId);

  float d = sqrt(bestDist);
  float core = 1.0 - smoothstep(r * 0.8, r, d);
  float mask = present * core;
  float edge = present * smoothstep(r * 0.55, r * 0.98, d);

  // Height: a rounded profile with slight randomness.
  float height = present * pow(saturate(core), mix(0.8, 1.6, bestId));

  return vec3(mask, edge, height);
}

void applyPebbles(inout vec3 albedo,
                  inout float roughness,
                  inout vec3 normal,
                  vec3 worldPos,
                  vec3 viewDir,
                  vec3 pebbleColor,
                  float pebbleRoughness,
                  float pebbleScale,
                  float pebbleDensity,
                  float blend,
                  float normalStrength,
                  float heightStrength) {
  // Tiny parallax-ish shift so pebbles feel raised when viewed at an angle.
  vec2 v = normalize(viewDir.xz + vec2(1e-4));
  vec3 m0 = pebbleMask(worldPos.xz, pebbleScale, pebbleDensity);
  vec2 shiftedXZ = worldPos.xz - v * (m0.z * heightStrength) * 0.35;

  vec3 m = pebbleMask(shiftedXZ, pebbleScale, pebbleDensity);
  float mask = m.x;
  float edge = m.y;
  float height = m.z;
  if (mask <= 0.0001) return;

  // Slight grime around edges.
  albedo *= 1.0 - edge * 0.14 * blend;

  // Blend albedo/roughness toward pebble properties.
  albedo = mix(albedo, pebbleColor, mask * blend);
  roughness = mix(roughness, pebbleRoughness, mask * blend);

  // Tiny sparkly highlight variation on pebbles.
  float sparkle = pow(valueNoise(worldPos.xz * 13.0 + viewDir.xz * 0.5), 12.0) * 0.15;
  albedo += sparkle * mask * blend;

  // Bump pebbles: build a pseudo height field normal from mask gradient.
  float eps = 0.15;
  float hx = pebbleMask(shiftedXZ + vec2(eps, 0.0), pebbleScale, pebbleDensity).z;
  float hz = pebbleMask(shiftedXZ + vec2(0.0, eps), pebbleScale, pebbleDensity).z;
  vec2 grad = vec2(hx - height, hz - height) / eps;
  vec3 bump = normalize(vec3(-grad.x * heightStrength, 1.0, -grad.y * heightStrength));
  normal = normalize(mix(normal, bump, mask * blend * normalStrength));
}
